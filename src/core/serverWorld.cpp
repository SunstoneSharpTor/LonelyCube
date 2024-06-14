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
            if (chunkOutOfRange) {
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

void ServerWorld::loadChunksAroundPlayers() {
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

void ServerWorld::loadChunk() {
    m_chunksToBeLoadedMtx.lock();
    if (!m_chunksToBeLoaded.empty()) {
        Position chunkPosition = m_chunksToBeLoaded.front();
        m_chunksToBeLoadedMtx.unlock();
        m_chunks.emplace(chunkPosition, chunkPosition);
        TerrainGen().generateTerrain(m_chunks.at(chunkPosition), m_seed);
        m_chunksBeingLoadedMtx.lock();
        for (auto& [playaerID, player] : m_players) {
            if (player.hasChunkLoaded(chunkPosition)) {
                m_chunks.at(chunkPosition).incrementPlayerCount();
            }
        }
        m_chunksBeingLoaded.erase(chunkPosition);
        m_chunksBeingLoadedMtx.unlock();
        if (m_singleplayer) {
            m_unmeshedChunksMtx.lock();
            m_unmeshedChunks.push(chunkPosition);
            m_unmeshedChunksMtx.unlock();
        }
    }
    else {
        m_chunksToBeLoadedMtx.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }
}

int ServerWorld::addPlayer(int* blockPosition, float* subBlockPosition, unsigned short renderDistance) {
    m_playersMtx.lock();
    m_players.emplace(std::piecewise_construct,
        std::forward_as_tuple(m_nextPlayerID),
        std::forward_as_tuple(m_nextPlayerID, blockPosition, subBlockPosition, renderDistance));
    m_nextPlayerID++;
    m_playersMtx.unlock();
    return m_nextPlayerID - 1;
}