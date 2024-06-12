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

#include "core/chunk.h"
#include "core/random.h"

ServerWorld::ServerWorld(bool multiplayer, unsigned long long seed) : m_multiplayer(multiplayer), m_seed(seed), m_nextPlayerID(0) {
    PCG_SeedRandom32(m_seed);
    seedNoise();
    
    m_numChunkLoadingThreads = std::max(1u, std::min(8u, std::thread::hardware_concurrency() - 1));
}

void ServerWorld::updatePlayerPos(int playerID, int* blockPosition, float* subBlockPosition) {
    m_players.at(playerID).updatePlayerPos(blockPosition, subBlockPosition);
}

void ServerWorld::loadChunks() {
    if (m_chunksToBeLoaded.size() < m_numChunkLoadingThreads) {
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
    }
}