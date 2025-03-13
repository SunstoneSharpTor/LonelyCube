/*
  Lonely Cube, a voxel game
  Copyright (C) 2024-2025 Bertie Cartwright

  Lonely Cube is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Lonely Cube is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "core/pch.h"

#include "enet/enet.h"

#include "core/block.h"
#include "core/chunk.h"
#include "core/chunkManager.h"
#include "core/compression.h"
#include "core/constants.h"
#include "core/entities/entityManager.h"
#include "core/log.h"
#include "core/packet.h"
#include "core/random.h"
#include "core/resourcePack.h"
#include "core/serverPlayer.h"
#include "core/terrainGen.h"
#include <mutex>
#include <string>

namespace lonelycube {

template<bool integrated>
class ServerWorld {
public:
    ChunkManager chunkManager;

private:
    uint64_t m_seed;
    uint16_t m_nextPlayerID;
    uint16_t m_numChunkLoadingThreads;
    uint64_t m_gameTick;
    ResourcePack m_resourcePack;
    std::chrono::time_point<std::chrono::steady_clock> m_timeOfLastTick;

    // World
    std::unordered_map<uint16_t, ServerPlayer> m_players;
    std::queue<IVec3> m_chunksToBeLoaded;
    std::unordered_set<IVec3> m_chunksBeingLoaded;

    EntityManager m_entityManager;

    // Synchronisation
    std::mutex m_chunksMtx;
    std::mutex m_playersMtx;
    std::mutex m_chunksToBeLoadedMtx;
    std::mutex m_chunksBeingLoadedMtx;
    std::mutex& m_networkingMtx;
    bool m_threadsWait;
    bool* m_threadWaiting;

    void disconnectPlayer(uint16_t playerID);

public:
    ServerWorld(uint64_t seed, std::mutex& networkingMtx);
    void tick();
    void addPlayer(int* blockPosition, float* subBlockPosition, uint16_t renderDistance, bool multiplayer);
    uint16_t addPlayer(int* blockPosition, float* subBlockPosition, uint16_t renderDistance, ENetPeer* peer);
    void updatePlayerPos(int playerID, const IVec3& blockPosition, const Vec3& subBlockPosition, bool unloadNeeded);
    ServerPlayer& getPlayer(int playerID) {
        return m_players.at(playerID);
    }
    std::unordered_map<uint16_t, ServerPlayer>& getPlayers() {
        return m_players;
    }
    void waitIfRequired(uint8_t threadNum);
    void pauseChunkLoaderThreads();
    void releaseChunkLoaderThreads();
    void findChunksToLoad();  // Must be called with m_chunksToBeLoadedMtx locked
    bool loadNextChunk(IVec3* chunkPosition);
    void loadChunkFromPacket(Packet<uint8_t, 9 * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE * constants::CHUNK_SIZE>& payload, IVec3& chunkPosition);
    void broadcastBlockReplaced(int* blockCoords, int blockType, int originalPlayerID);
    bool getNextLoadedChunkPosition(IVec3* chunkPosition);
    float getTimeSinceLastTick();
    inline int8_t getNumChunkLoaderThreads() {
        return m_numChunkLoadingThreads;
    }
    inline uint64_t getTickNum() {
        return m_gameTick;
    }
    inline ResourcePack& getResourcePack() {
        return m_resourcePack;
    }
    inline EntityManager& getEntityManager()
    {
        return m_entityManager;
    }
    void setPlayerChunkLoadingTarget(int playerID, uint64_t chunkRequestNum, int target, int bufferSize);
    bool updateClientChunkLoadingTarget();
};

template<bool integrated>
ServerWorld<integrated>::ServerWorld(uint64_t seed, std::mutex& networkingMtx)
    : m_seed(seed), m_nextPlayerID(0), m_gameTick(0), m_resourcePack("res/resourcePack"),
    m_entityManager(10000, chunkManager, m_resourcePack), m_networkingMtx(networkingMtx),
    m_threadsWait(false)
{
    PCG_SeedRandom32(m_seed);
    seedNoise();
    m_players.reserve(32);

    m_numChunkLoadingThreads = std::max(1u, std::min(32u, std::thread::hardware_concurrency()));
    m_threadWaiting = new bool[m_numChunkLoadingThreads];
    std::fill(m_threadWaiting, m_threadWaiting + m_numChunkLoadingThreads, false);
}

template<bool integrated>
void ServerWorld<integrated>::updatePlayerPos(
    int playerID, const IVec3& blockPosition, const Vec3& subBlockPosition, bool unloadNeeded
) {
    m_playersMtx.lock();
    ServerPlayer& player = m_players.at(playerID);
    player.updatePlayerPos(blockPosition, subBlockPosition);
    if (unloadNeeded) {
        m_chunksMtx.lock();
        m_chunksToBeLoadedMtx.lock();
        m_chunksBeingLoadedMtx.lock();
        // If the player has moved chunk, remove all the chunks that are out of
        // render distance from the set of loaded chunks
        IVec3 chunkPosition;
        bool chunkOutOfRange;
        player.beginUnloadingChunks();
        while (player.checkIfNextChunkShouldUnload(&chunkPosition, &chunkOutOfRange)) {
            if (chunkOutOfRange) {
                auto it = chunkManager.getWorldChunks().find(chunkPosition);
                it->second.decrementPlayerCount();
                if (it->second.hasNoPlayers()) {
                    it->second.unload();
                    chunkManager.getWorldChunks().erase(it);
                }
            }
        }
        m_chunksBeingLoadedMtx.unlock();
        m_chunksToBeLoadedMtx.unlock();
        m_chunksMtx.unlock();
    }
    m_playersMtx.unlock();
}

template<bool integrated>
void ServerWorld<integrated>::findChunksToLoad() {
    std::lock_guard<std::mutex> lock1(m_playersMtx);
    std::lock_guard<std::mutex> lock2(m_chunksBeingLoadedMtx);
    std::lock_guard<std::mutex> lock3(m_chunksMtx);
    for (auto& [playerID, player] : m_players) {
        if (player.updateNextUnloadedChunk() && (player.wantsMoreChunks() || integrated)) {
            int chunkPosition[3];
            player.getNextChunkCoords(chunkPosition, m_gameTick);
            auto it = chunkManager.getWorldChunks().find(IVec3(chunkPosition));
            if (it != chunkManager.getWorldChunks().end()) {
                it->second.incrementPlayerCount();
                if (!integrated) {
                    Packet<uint8_t, 9 * constants::CHUNK_SIZE * constants::CHUNK_SIZE
                        * constants::CHUNK_SIZE> payload(0, PacketType::ChunkSent, 0);
                    Compression::compressChunk(payload, it->second);
                    payload.setPeerID(playerID);
                    ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
                    std::lock_guard<std::mutex> lock(m_networkingMtx);
                    enet_peer_send(player.getPeer(), 0, packet);
                }
            }
            else if (!m_chunksBeingLoaded.contains(IVec3(chunkPosition))) {
                m_chunksToBeLoaded.emplace(chunkPosition);
                m_chunksBeingLoaded.emplace(chunkPosition);
            }
        }
    }
}

template<bool integrated>
bool ServerWorld<integrated>::loadNextChunk(IVec3* chunkPosition) {
    m_chunksToBeLoadedMtx.lock();
    if (m_chunksToBeLoaded.empty())
        findChunksToLoad();
    if (!m_chunksToBeLoaded.empty()) {
        *chunkPosition = m_chunksToBeLoaded.front();
        m_chunksToBeLoaded.pop();
        m_chunksToBeLoadedMtx.unlock();
        m_chunksMtx.lock();
        Chunk::s_checkingNeighbourSkyRelightsMtx.lock();
        chunkManager.getWorldChunks()[*chunkPosition] = { *chunkPosition };
        Chunk::s_checkingNeighbourSkyRelightsMtx.unlock();
        Chunk& chunk = chunkManager.getChunk(*chunkPosition);
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
        chunk.setSkyLightBeingRelit(false);
        chunk.setBlockLightBeingRelit(false);
        for (auto& [playerID, player] : m_players) {
            if (player.hasChunkLoaded(*chunkPosition)) {
                chunk.incrementPlayerCount();
                if (!integrated) {
                    payload.setPeerID(playerID);
                    ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
                    std::lock_guard<std::mutex> lock(m_networkingMtx);
                    enet_peer_send(player.getPeer(), 0, packet);
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
    Chunk::s_checkingNeighbourSkyRelightsMtx.lock();
    chunkManager.getWorldChunks()[chunkPosition] = { chunkPosition };
    Chunk::s_checkingNeighbourSkyRelightsMtx.unlock();
    Chunk& chunk = chunkManager.getChunk(chunkPosition);
    m_chunksMtx.unlock();
    Compression::decompressChunk(payload, chunk);
    chunk.setSkyLightBeingRelit(false);
    chunk.setBlockLightBeingRelit(false);
    if (integrated)
        m_players.at(0).setChunkLoaded(chunkPosition, m_gameTick);
}

// Overload used by the physical server
template<bool integrated>
uint16_t ServerWorld<integrated>::addPlayer(int* blockPosition, float* subBlockPosition, uint16_t renderDistance, ENetPeer* peer) {
    m_playersMtx.lock();
    uint16_t playerID = m_nextPlayerID;
    m_players[playerID] = { m_nextPlayerID, blockPosition, subBlockPosition, renderDistance, peer, m_gameTick };
    m_nextPlayerID = 0;
    while (m_players.contains(m_nextPlayerID))
        m_nextPlayerID++;
    m_playersMtx.unlock();
    return playerID;
}

// Overload used by the integrated server
template<bool integrated>
void ServerWorld<integrated>::addPlayer(int* blockPosition, float* subBlockPosition, uint16_t renderDistance, bool multiplayer) {
    m_playersMtx.lock();
    m_players[0] = { m_nextPlayerID, blockPosition, subBlockPosition, renderDistance, multiplayer };
    m_playersMtx.unlock();
}

template<bool integrated>
void ServerWorld<integrated>::disconnectPlayer(uint16_t playerID) {
    LOG(std::to_string(chunkManager.getWorldChunks().size()));
    pauseChunkLoaderThreads();

    std::lock_guard<std::mutex> lock1(m_playersMtx);
    std::lock_guard<std::mutex> lock2(m_chunksMtx);
    std::lock_guard<std::mutex> lock3(m_chunksToBeLoadedMtx);
    std::lock_guard<std::mutex> lock4(m_chunksBeingLoadedMtx);
    ServerPlayer& player = m_players.at(playerID);

    int blockPosition[3];
    player.getChunkPosition(blockPosition);
    blockPosition[0] += player.getRenderDistance() * 4 * constants::CHUNK_SIZE;
    float subBlockPosition[3] = { 0.0f, 0.0f, 0.0f };
    player.updatePlayerPos(blockPosition, subBlockPosition);

    IVec3 chunkPosition;
    bool chunkOutOfRange;
    int i = 0;
    player.beginUnloadingChunks();
    while (player.checkIfNextChunkShouldUnload(&chunkPosition, &chunkOutOfRange)) {
        if (chunkManager.chunkLoaded(chunkPosition)) {
            auto it = chunkManager.getWorldChunks().find(chunkPosition);
            if (it != chunkManager.getWorldChunks().end()) {
                it->second.decrementPlayerCount();
                if (it->second.hasNoPlayers()) {
                    it->second.unload();
                    chunkManager.getWorldChunks().erase(it);
                }
            }
        }
        i++;
    }
    LOG(std::to_string(i) + " chunks checked");
    while (!m_chunksToBeLoaded.empty()) {
        m_chunksToBeLoaded.pop();
    }
    m_chunksBeingLoaded.clear();

    releaseChunkLoaderThreads();
    LOG(std::to_string(playerID) + " disconnected");
    LOG(std::to_string(chunkManager.getWorldChunks().size()));
}

template<bool integrated>
void ServerWorld<integrated>::waitIfRequired(uint8_t threadNum) {
    while (m_threadsWait) {
        while (m_threadsWait) {
            m_threadWaiting[threadNum] = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            m_threadWaiting[threadNum] = false;
        }
    }
}

template<bool integrated>
void ServerWorld<integrated>::pauseChunkLoaderThreads() {
    // Wait for all the chunk loader threads to finish their jobs
    m_threadsWait = true;
    bool allThreadsWaiting = false;
    while (!allThreadsWaiting) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        for (int8_t threadNum = 0; threadNum < m_numChunkLoadingThreads; threadNum++) {
            allThreadsWaiting |= !m_threadWaiting[threadNum];
        }
        allThreadsWaiting = !allThreadsWaiting;
    }
}

template<bool integrated>
void ServerWorld<integrated>::releaseChunkLoaderThreads() {
    m_threadsWait = false;
}

template<bool integrated>
void ServerWorld<integrated>::tick() {
    m_timeOfLastTick = std::chrono::steady_clock::now();
    m_entityManager.tick();

    if (!integrated) {
        auto it = m_players.begin();
        while (it != m_players.end()) {
            if (m_gameTick - it->second.getLastPacketTick() > 40) {
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
    std::lock_guard<std::mutex> lock(m_networkingMtx);
    for (auto& [playerID, player] : m_players) {
        if ((playerID != originalPlayerID) && (player.hasChunkLoaded(chunkPosition))) {
            ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(player.getPeer(), 0, packet);
        }
    }
}

template<bool integrated>
float ServerWorld<integrated>::getTimeSinceLastTick()
{
    auto currentTime = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_timeOfLastTick).count() * 0.001f;
}

template<bool integrated>
bool ServerWorld<integrated>::updateClientChunkLoadingTarget()
{
    std::lock_guard<std::mutex> lock(m_playersMtx);
    return m_players.at(0).updateChunkLoadingTarget();
}

template<bool integrated>
void ServerWorld<integrated>::setPlayerChunkLoadingTarget(
    int playerID, uint64_t chunkRequestNum, int target, int bufferSize
) {
    std::lock_guard<std::mutex> lock(m_playersMtx);
    ServerPlayer& player = getPlayer(playerID);
    if (chunkRequestNum > player.getNumChunkRequests())
    {
        player.setNumChunkRequests(chunkRequestNum);
        player.setTargetBufferSize(bufferSize);
        player.setChunkLoadingTarget(target, m_gameTick);
        LOG("Chunk request for " + std::to_string(target));
    }
}

}  // namespace lonelycube
