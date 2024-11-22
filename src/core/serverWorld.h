/*
  Lonely Cube, a voxel game
  Copyright (C) 2024 Bertie Cartwright

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "core/entities/entityManager.h"
#include "core/pch.h"

#include "enet/enet.h"

#include "core/block.h"
#include "core/chunk.h"
#include "core/compression.h"
#include "core/constants.h"
#include "core/packet.h"
#include "core/random.h"
#include "core/resourcePack.h"
#include "core/serverPlayer.h"
#include "core/terrainGen.h"

template<bool integrated>
class ServerWorld {
private:
    uint64_t m_seed;
    uint16_t m_nextPlayerID;
    uint16_t m_numChunkLoadingThreads;
    uint32_t m_gameTick;
    ResourcePack m_resourcePack;

    // World
    std::unordered_map<IVec3, Chunk> m_chunks;
    std::unordered_map<uint16_t, ServerPlayer> m_players;
    std::queue<IVec3> m_chunksToBeLoaded;
    std::unordered_set<IVec3> m_chunksBeingLoaded;

    EntityManager m_entityManager;

    // Synchronisation
    std::mutex m_chunksMtx;
    std::mutex m_playersMtx;
    std::mutex m_chunksToBeLoadedMtx;
    std::mutex m_chunksBeingLoadedMtx;
    std::mutex m_unmeshedChunksMtx;
    std::mutex m_threadsWaitMtx;
    std::condition_variable m_threadsWaitCV;
    bool m_threadsWait;
    bool* m_threadWaiting;

    void disconnectPlayer(uint16_t playerID);

public:
    ServerWorld(uint64_t seed);
    void tick();
    void addPlayer(int* blockPosition, float* subBlockPosition, uint16_t renderDistance);
    uint16_t addPlayer(int* blockPosition, float* subBlockPosition, uint16_t renderDistance, ENetPeer* peer);
    void updatePlayerPos(int playerID, int* blockPosition, float* subBlockPosition, bool unloadNeeded);
    ServerPlayer& getPlayer(int playerID) {
        return m_players.at(playerID);
    }
    std::unordered_map<uint16_t, ServerPlayer>& getPlayers() {
        return m_players;
    }
    void waitIfRequired(uint8_t threadNum);
    void pauseChunkLoaderThreads();
    void releaseChunkLoaderThreads();
    void findChunksToLoad();
    bool loadChunk(IVec3* chunkPosition);
    void loadChunkFromPacket(Packet<uint8_t, 9 * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE * constants::CHUNK_SIZE>& payload, IVec3& chunkPosition);
    void broadcastBlockReplaced(int* blockCoords, int blockType, int originalPlayerID);
    bool getNextLoadedChunkPosition(IVec3* chunkPosition);
    uint8_t getBlock(const IVec3& position) const;
    void setBlock(const IVec3& position, uint8_t blockType);
    uint8_t getSkyLight(const IVec3& position) const;
    uint8_t getBlockLight(const IVec3& position) const;
    inline Chunk& getChunk(const IVec3& chunkPosition) {
        return m_chunks.at(chunkPosition);
    }
    inline bool chunkLoaded(const IVec3& chunkPosition) {
        return m_chunks.contains(chunkPosition);
    }
    inline std::unordered_map<IVec3, Chunk>& getWorldChunks() {
        return m_chunks;
    }
    inline int8_t getNumChunkLoaderThreads() {
        return m_numChunkLoadingThreads;
    }
    inline uint32_t getTickNum() {
        return m_gameTick;
    }
    inline ResourcePack& getResourcePack() {
        return m_resourcePack;
    }
    inline EntityManager& getEntityManager()
    {
        return m_entityManager;
    }
};

template<bool integrated>
ServerWorld<integrated>::ServerWorld(uint64_t seed) : m_seed(seed), m_nextPlayerID(0),
    m_gameTick(0), m_resourcePack("res/resourcePack"), m_entityManager(10000, m_resourcePack),
    m_threadsWait(false)
{
    PCG_SeedRandom32(m_seed);
    seedNoise();
    // TODO:
    // set this reserved number for m_chunks to be dependant on render distance for singleplayer
    m_chunks.reserve(16777214);
    m_players.reserve(32);

    m_numChunkLoadingThreads = std::max(1u, std::min(32u, std::thread::hardware_concurrency()));
    m_threadWaiting = new bool[m_numChunkLoadingThreads];
    std::fill(m_threadWaiting, m_threadWaiting + m_numChunkLoadingThreads, false);
}

template<bool integrated>
void ServerWorld<integrated>::updatePlayerPos(int playerID, int* blockPosition, float* subBlockPosition, bool unloadNeeded) {
    int currentPosition[3];
    m_players.at(playerID).getChunkPosition(currentPosition);
    if (unloadNeeded) {
        m_playersMtx.lock();
        m_chunksMtx.lock();
        m_chunksToBeLoadedMtx.lock();
        m_chunksBeingLoadedMtx.lock();
        ServerPlayer& player = m_players.at(playerID);
        player.updatePlayerPos(blockPosition, subBlockPosition);
        // If the player has moved chunk, remove all the chunks that are out of
        // render distance from the set of loaded chunks
        IVec3 chunkPosition;
        bool chunkOutOfRange;
        while (player.decrementNextChunk(&chunkPosition, &chunkOutOfRange)) {
            if (chunkOutOfRange && m_chunks.contains(chunkPosition)) {
                m_chunks.at(chunkPosition).decrementPlayerCount();
                if (m_chunks.at(chunkPosition).hasNoPlayers()) {
                    m_chunks.at(chunkPosition).unload();
                    m_chunks.erase(chunkPosition);
                }
            }
        }
        while (!m_chunksToBeLoaded.empty()) {
            m_chunksToBeLoaded.pop();
        }
        m_chunksBeingLoaded.clear();
        m_chunksBeingLoadedMtx.unlock();
        m_chunksToBeLoadedMtx.unlock();
        m_chunksMtx.unlock();
        m_playersMtx.unlock();
    }
}

template<bool integrated>
void ServerWorld<integrated>::findChunksToLoad() {
    if (m_chunksToBeLoaded.size() > 0) {
        return;
    }
    m_playersMtx.lock();
    m_chunksToBeLoadedMtx.lock();
    m_chunksBeingLoadedMtx.lock();
    m_chunksMtx.lock();
    for (auto& [playerID, player] : m_players) {
        if (!player.allChunksLoaded()) {
            int chunkPosition[3];
            player.getNextChunkCoords(chunkPosition);
            auto it = m_chunks.find(IVec3(chunkPosition));
            if (it != m_chunks.end()) {
                it->second.incrementPlayerCount();
                if (!integrated) {
                    Packet<uint8_t, 9 * constants::CHUNK_SIZE * constants::CHUNK_SIZE
                        * constants::CHUNK_SIZE> payload(0, PacketType::ChunkSent, 0);
                        Compression::compressChunk(payload, it->second);
                    payload.setPeerID(playerID);
                    ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
                    !enet_peer_send(player.getPeer(), 0, packet);
                }
            }
            else if (!m_chunksBeingLoaded.contains(IVec3(chunkPosition))) {
                m_chunksToBeLoaded.emplace(chunkPosition);
                m_chunksBeingLoaded.emplace(chunkPosition);
            }
        }
    }
    m_chunksMtx.unlock();
    m_chunksBeingLoadedMtx.unlock();
    m_chunksToBeLoadedMtx.unlock();
    m_playersMtx.unlock();
}

template<bool integrated>
bool ServerWorld<integrated>::loadChunk(IVec3* chunkPosition) {
    findChunksToLoad();
    m_chunksToBeLoadedMtx.lock();
    if (!m_chunksToBeLoaded.empty()) {
        *chunkPosition = m_chunksToBeLoaded.front();
        m_chunksToBeLoaded.pop();
        m_chunksToBeLoadedMtx.unlock();
        m_chunksMtx.lock();
        m_chunks[*chunkPosition] = { *chunkPosition };
        Chunk& chunk = m_chunks.at(*chunkPosition);
        m_chunksMtx.unlock();
        TerrainGen().generateTerrain(chunk, m_seed);
        m_chunksBeingLoadedMtx.lock();
        m_chunksBeingLoaded.erase(*chunkPosition);
        m_chunksBeingLoadedMtx.unlock();
        Packet<uint8_t, 9 * constants::CHUNK_SIZE * constants::CHUNK_SIZE
            * constants::CHUNK_SIZE> payload(0, PacketType::ChunkSent, 0);
        if (!integrated) {
            Compression::compressChunk(payload, chunk);
        }
        for (auto& [playerID, player] : m_players) {
            if (player.hasChunkLoaded(*chunkPosition)) {
                chunk.incrementPlayerCount();
                if (!integrated) {
                    payload.setPeerID(playerID);
                    ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
                    if (!enet_peer_send(player.getPeer(), 0, packet));
                }
            }
        }
        return true;
    }
    else {
        m_chunksToBeLoadedMtx.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
        return false;
    }
}

template<bool integrated>
void ServerWorld<integrated>::loadChunkFromPacket(Packet<uint8_t, 9 * constants::CHUNK_SIZE *
    constants::CHUNK_SIZE * constants::CHUNK_SIZE>& payload, IVec3& chunkPosition) {
    Compression::getChunkPosition(payload, chunkPosition);
    m_chunksMtx.lock();
    m_chunks[chunkPosition] = { chunkPosition };
    Chunk& chunk = m_chunks.at(chunkPosition);
    m_chunksMtx.unlock();
    Compression::decompressChunk(payload, chunk);
}

template<bool integrated>
uint16_t ServerWorld<integrated>::addPlayer(int* blockPosition, float* subBlockPosition, uint16_t renderDistance, ENetPeer* peer) {
    m_playersMtx.lock();
    uint16_t playerID = m_nextPlayerID;
    m_players[playerID] = { m_nextPlayerID, blockPosition, subBlockPosition, renderDistance, peer, m_gameTick };
    m_nextPlayerID++;
    m_playersMtx.unlock();
    return playerID;
}

template<bool integrated>
void ServerWorld<integrated>::addPlayer(int* blockPosition, float* subBlockPosition, uint16_t renderDistance) {
    m_playersMtx.lock();
    m_players[m_nextPlayerID] = { m_nextPlayerID, blockPosition, subBlockPosition, renderDistance };
    m_playersMtx.unlock();
}

template<bool integrated>
void ServerWorld<integrated>::disconnectPlayer(uint16_t playerID) {
    std::cout << m_chunks.size() << std::endl;
    pauseChunkLoaderThreads();

    // Remove the player from all chunks that it had loaded
    m_playersMtx.lock();
    m_chunksMtx.lock();
    m_chunksToBeLoadedMtx.lock();
    m_chunksBeingLoadedMtx.lock();
    ServerPlayer& player = m_players.at(playerID);

    int blockPosition[3];
    player.getChunkPosition(blockPosition);
    for (int i = 0; i < 3; i++) {
        blockPosition[i] = (blockPosition[i] + player.getRenderDistance() * 2) * constants::CHUNK_SIZE;
    }
    float subBlockPosition[3] = { 0.0f, 0.0f, 0.0f };
    player.updatePlayerPos(blockPosition, subBlockPosition);

    IVec3 chunkPosition;
    bool chunkOutOfRange;
    int i = 0;
    while (player.decrementNextChunk(&chunkPosition, &chunkOutOfRange)) {
        if (m_chunks.contains(chunkPosition)) {
            m_chunks.at(chunkPosition).decrementPlayerCount();
            if (m_chunks.at(chunkPosition).hasNoPlayers()) {
                m_chunks.at(chunkPosition).unload();
                m_chunks.erase(chunkPosition);
            }
        }
        i++;
    }
    std::cout << i << " chunks checked\n";
    while (!m_chunksToBeLoaded.empty()) {
        m_chunksToBeLoaded.pop();
    }
    m_chunksBeingLoaded.clear();
    m_chunksBeingLoadedMtx.unlock();
    m_chunksToBeLoadedMtx.unlock();
    m_chunksMtx.unlock();
    m_playersMtx.unlock();

    releaseChunkLoaderThreads();
    std::cout << playerID << " disconnected\n";
    std::cout << m_chunks.size() << std::endl;
}

template<bool integrated>
uint8_t ServerWorld<integrated>::getBlock(const IVec3& position) const {
    IVec3 chunkPosition = Chunk::getChunkCoords(position);
    IVec3 chunkBlockCoords = IVec3(
        position.x - chunkPosition.x * constants::CHUNK_SIZE,
        position.y - chunkPosition.y * constants::CHUNK_SIZE,
        position.z - chunkPosition.z * constants::CHUNK_SIZE);
    uint32_t chunkBlockNum = chunkBlockCoords.y * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        + chunkBlockCoords.z * constants::CHUNK_SIZE + chunkBlockCoords.x;

    auto chunkIterator = m_chunks.find(chunkPosition);

    if (chunkIterator == m_chunks.end()) {
        return 0;
    }

    return chunkIterator->second.getBlock(chunkBlockNum);
}

template<bool integrated>
void ServerWorld<integrated>::setBlock(const IVec3& position, uint8_t blockType) {
    IVec3 chunkPosition = Chunk::getChunkCoords(position);
    IVec3 chunkBlockCoords = IVec3(
        position.x - chunkPosition.x * constants::CHUNK_SIZE,
        position.y - chunkPosition.y * constants::CHUNK_SIZE,
        position.z - chunkPosition.z * constants::CHUNK_SIZE);
    uint32_t chunkBlockNum = chunkBlockCoords.y * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        + chunkBlockCoords.z * constants::CHUNK_SIZE + chunkBlockCoords.x;

    auto chunkIterator = m_chunks.find(chunkPosition);

    if (chunkIterator == m_chunks.end()) {
        return;
    }

    if (blockType == air)
    {
        m_entityManager.addItem(
            chunkIterator->second.getBlock(chunkBlockNum), position, Vec3(0.5f, 0.5f, 0.5f)
        );
    }

    chunkIterator->second.setBlock(chunkBlockNum, blockType);
    chunkIterator->second.compressBlocks();
}

template<bool integrated>
uint8_t ServerWorld<integrated>::getSkyLight(const IVec3& position) const {
    IVec3 chunkPosition = Chunk::getChunkCoords(position);
    IVec3 chunkBlockCoords = IVec3(
        position.x - chunkPosition.x * constants::CHUNK_SIZE,
        position.y - chunkPosition.y * constants::CHUNK_SIZE,
        position.z - chunkPosition.z * constants::CHUNK_SIZE);
    uint32_t chunkBlockNum = chunkBlockCoords.y * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        + chunkBlockCoords.z * constants::CHUNK_SIZE + chunkBlockCoords.x;

    auto chunkIterator = m_chunks.find(chunkPosition);

    if (chunkIterator == m_chunks.end()) {
        return 0;
    }

    return chunkIterator->second.getSkyLight(chunkBlockNum);
}

template<bool integrated>
uint8_t ServerWorld<integrated>::getBlockLight(const IVec3& position) const {
    IVec3 chunkPosition = Chunk::getChunkCoords(position);
    IVec3 chunkBlockCoords = IVec3(
        position.x - chunkPosition.x * constants::CHUNK_SIZE,
        position.y - chunkPosition.y * constants::CHUNK_SIZE,
        position.z - chunkPosition.z * constants::CHUNK_SIZE);
    uint32_t chunkBlockNum = chunkBlockCoords.y * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        + chunkBlockCoords.z * constants::CHUNK_SIZE + chunkBlockCoords.x;

    auto chunkIterator = m_chunks.find(chunkPosition);

    if (chunkIterator == m_chunks.end()) {
        return 0;
    }

    return chunkIterator->second.getBlockLight(chunkBlockNum);
}

template<bool integrated>
void ServerWorld<integrated>::waitIfRequired(uint8_t threadNum) {
    while (m_threadsWait) {
        m_threadWaiting[threadNum] = true;
    m_threadsWaitCV.notify_all();
        // locking 
        std::unique_lock<std::mutex> lock(m_threadsWaitMtx);
        // waiting 
        while (m_threadsWait) {
            m_threadsWaitCV.wait(lock);
        }
        m_threadWaiting[threadNum] = false;
    }
}

template<bool integrated>
void ServerWorld<integrated>::pauseChunkLoaderThreads() {
    // Wait for all the chunk loader threads to finish their jobs
    m_threadsWait = true;
    bool allThreadsWaiting = false;
    while (!allThreadsWaiting) {
        std::unique_lock<std::mutex> lock(m_threadsWaitMtx);
        m_threadsWaitCV.wait(lock);
        for (int8_t threadNum = 0; threadNum < m_numChunkLoadingThreads; threadNum++) {
            allThreadsWaiting |= !m_threadWaiting[threadNum];
        }
        allThreadsWaiting = !allThreadsWaiting;
    }
}

template<bool integrated>
void ServerWorld<integrated>::releaseChunkLoaderThreads() {
    // Allow chunk loaded threads to continue work
    m_threadsWait = false;
    // lock release 
    std::lock_guard<std::mutex> lock(m_threadsWaitMtx);
    // notify consumer when done
    m_threadsWaitCV.notify_all();
}

template<bool integrated>
void ServerWorld<integrated>::tick() {
    if (!integrated) {
        auto it = m_players.begin();
        while (it != m_players.end()) {
            if (m_gameTick - it->second.getLastPacketTick() > 20) {
                disconnectPlayer(it->first);
                it = m_players.erase(it);
            }
            else {
                it++;
            }
        }
    }
    m_gameTick++;
}

template<bool integrated>
void ServerWorld<integrated>::broadcastBlockReplaced(int* blockCoords, int blockType, int originalPlayerID) {
    Packet<int, 4> payload(0, PacketType::BlockReplaced, 4);
    for (int i = 0; i < 3; i++) {
        payload[i] = blockCoords[i];
    }
    payload[3] = blockType;
    IVec3 chunkPosition = Chunk::getChunkCoords(blockCoords);
    for (auto& [playerID, player] : m_players) {
        if ((playerID != originalPlayerID) && (player.hasChunkLoaded(chunkPosition))) {
            ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(player.getPeer(), 0, packet);
        }
    }
}
