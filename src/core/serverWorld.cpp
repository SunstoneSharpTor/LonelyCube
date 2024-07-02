/*
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

#include "core/serverWorld.h"

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "core/chunk.h"
#include "core/random.h"
#include "core/terrainGen.h"

ServerWorld::ServerWorld(bool singleplayer, unsigned long long seed) : m_singleplayer(singleplayer), m_seed(seed), m_nextPlayerID(0) {
    PCG_SeedRandom32(m_seed);
    seedNoise();
    // TODO:
    // set this reserved number for m_chunks to be dependant on render distance for singleplayer
    m_chunks.reserve(16777214);
    m_players.reserve(32);

    m_numChunkLoadingThreads = std::max(1u, std::min(8u, std::thread::hardware_concurrency() - 1));
}

void ServerWorld::updatePlayerPos(int playerID, int* blockPosition, float* subBlockPosition) {
    m_playersMtx.lock();
    m_chunksMtx.lock();
    int lastPlayerChunkPosition[3];
    m_players.at(playerID).getChunkPosition(lastPlayerChunkPosition);
    m_players.at(playerID).updatePlayerPos(blockPosition, subBlockPosition);
    int newPlayerChunkPosition[3];
    m_players.at(playerID).getChunkPosition(newPlayerChunkPosition);
    // If the player has moved chunk, remove all the chunks that are out of
    // render distance from the set of loaded chunks
    if (newPlayerChunkPosition[0] != lastPlayerChunkPosition[0]
        || newPlayerChunkPosition[1] != lastPlayerChunkPosition[1]
        || newPlayerChunkPosition[2] != lastPlayerChunkPosition[2]) {
        Position chunkPosition;
        bool chunkOutOfRange;
        while (m_players.at(playerID).decrementNextChunk(&chunkPosition, &chunkOutOfRange)) {
            if (chunkOutOfRange && m_chunks.contains(chunkPosition)) {
                m_chunks.at(chunkPosition).decrementPlayerCount();
                if (m_chunks.at(chunkPosition).hasNoPlayers()) {
                    m_chunks.at(chunkPosition).unload();
                    m_chunks.erase(chunkPosition);
                }
            }
        }
    }
    m_chunksMtx.unlock();
    m_playersMtx.unlock();
}

void ServerWorld::findChunksToLoad() {
    if (m_chunksToBeLoaded.size() < m_numChunkLoadingThreads) {
        m_playersMtx.lock();
        m_chunksToBeLoadedMtx.lock();
        m_chunksBeingLoadedMtx.lock();
        m_chunksMtx.lock();
        for (auto& [playaerID, player] : m_players) {
            if (!player.allChunksLoaded()) {
                int chunkPosition[3];
                player.getNextChunkCoords(chunkPosition);
                if (m_chunks.contains(Position(chunkPosition))) {
                    m_chunks[chunkPosition].incrementPlayerCount();
                }
                else if (!m_chunksBeingLoaded.contains(Position(chunkPosition))) {
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
}

bool ServerWorld::loadChunk(Position* chunkPosition) {
    m_chunksToBeLoadedMtx.lock();
    if (!m_chunksToBeLoaded.empty()) {
        *chunkPosition = m_chunksToBeLoaded.front();
        m_chunksToBeLoaded.pop();
        m_chunksToBeLoadedMtx.unlock();
        m_chunksMtx.lock();
        m_chunks[*chunkPosition] = { *chunkPosition, &m_chunks };
        m_chunksMtx.unlock();
        TerrainGen().generateTerrain(m_chunks.at(*chunkPosition), m_seed);
        m_chunksBeingLoadedMtx.lock();
        for (auto& [playaerID, player] : m_players) {
            if (player.hasChunkLoaded(*chunkPosition) && m_chunks.contains(*chunkPosition)) {
                m_chunks.at(*chunkPosition).incrementPlayerCount();
            }
        }
        m_chunksBeingLoaded.erase(*chunkPosition);
        m_chunksBeingLoadedMtx.unlock();
        if (m_singleplayer) {
            m_unmeshedChunksMtx.lock();
            m_unmeshedChunks.push(*chunkPosition);
            m_unmeshedChunksMtx.unlock();
        }
        return true;
    }
    else {
        m_chunksToBeLoadedMtx.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
        return false;
    }
}

int ServerWorld::addPlayer(int* blockPosition, float* subBlockPosition, unsigned short renderDistance) {
    m_playersMtx.lock();
    m_players[m_nextPlayerID] = ServerPlayer { m_nextPlayerID, blockPosition, subBlockPosition, renderDistance };
    m_nextPlayerID++;
    m_playersMtx.unlock();
    return m_nextPlayerID - 1;
}

bool ServerWorld::getNextLoadedChunkPosition(Position* chunkPosition) {
    m_unmeshedChunksMtx.lock();
    if (m_unmeshedChunks.empty()) {
        m_unmeshedChunksMtx.unlock();
        return false;
    }
    else {
        *chunkPosition = m_unmeshedChunks.front();
        m_unmeshedChunks.pop();
        m_unmeshedChunksMtx.unlock();
        return true;
    }
}


unsigned char ServerWorld::getBlock(const Position& position) const {
    Position chunkPosition;
    Position chunkBlockCoords;
    chunkPosition.x = -1 * (position.x < 0) + position.x / constants::CHUNK_SIZE;
    chunkPosition.y = -1 * (position.y < 0) + position.y / constants::CHUNK_SIZE;
    chunkPosition.z = -1 * (position.z < 0) + position.z / constants::CHUNK_SIZE;
    chunkBlockCoords.x = position.x - chunkPosition.x * constants::CHUNK_SIZE;
    chunkBlockCoords.y = position.y - chunkPosition.y * constants::CHUNK_SIZE;
    chunkBlockCoords.z = position.z - chunkPosition.z * constants::CHUNK_SIZE;
    unsigned int chunkBlockNum = chunkBlockCoords.y * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        + chunkBlockCoords.z * constants::CHUNK_SIZE + chunkBlockCoords.x;

    auto chunkIterator = m_chunks.find(chunkPosition);

    if (chunkIterator == m_chunks.end()) {
        return 0;
    }

    return chunkIterator->second.getBlock(chunkBlockNum);
}


void ServerWorld::setBlock(const Position& position, unsigned char blockType) {
    Position chunkPosition;
    Position chunkBlockCoords;
    chunkPosition.x = -1 * (position.x < 0) + position.x / constants::CHUNK_SIZE;
    chunkPosition.y = -1 * (position.y < 0) + position.y / constants::CHUNK_SIZE;
    chunkPosition.z = -1 * (position.z < 0) + position.z / constants::CHUNK_SIZE;
    chunkBlockCoords.x = position.x - chunkPosition.x * constants::CHUNK_SIZE;
    chunkBlockCoords.y = position.y - chunkPosition.y * constants::CHUNK_SIZE;
    chunkBlockCoords.z = position.z - chunkPosition.z * constants::CHUNK_SIZE;
    unsigned int chunkBlockNum = chunkBlockCoords.y * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        + chunkBlockCoords.z * constants::CHUNK_SIZE + chunkBlockCoords.x;

    auto chunkIterator = m_chunks.find(chunkPosition);

    if (chunkIterator == m_chunks.end()) {
        return;
    }

    chunkIterator->second.setBlock(chunkBlockNum, blockType);
}

void ServerWorld::addToUnmeshedChunks(const Position& chunkPosition) {
    m_unmeshedChunksMtx.lock();
    m_unmeshedChunks.push(chunkPosition);
    m_unmeshedChunksMtx.unlock();
}