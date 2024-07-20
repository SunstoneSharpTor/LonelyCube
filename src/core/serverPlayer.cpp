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

#include "core/serverPlayer.h"

#include "core/pch.h"

#include "core/chunk.h"
#include "core/constants.h"
#include "core/position.h"

void ServerPlayer::initNumChunks() {
    m_minUnloadedChunkDistance = (m_renderDistance + 1) * (m_renderDistance + 1);
    m_numChunks = 0;
    for (int x = -m_renderDistance; x <= m_renderDistance; x++) {
        for (int y = -m_renderDistance; y <= m_renderDistance; y++) {
            for (int z = -m_renderDistance; z <= m_renderDistance; z++) {
                if (x * x + y * y + z * z < m_minUnloadedChunkDistance) {
                    m_numChunks++;
                }
            }
        }
    }
    m_unloadedChunks = new Position[m_numChunks];
}

void ServerPlayer::initChunkPositions() {
    int i = 0;
    for (int x = -m_renderDistance; x <= m_renderDistance; x++) {
        for (int y = -m_renderDistance; y <= m_renderDistance; y++) {
            for (int z = -m_renderDistance; z <= m_renderDistance; z++) {
                if (x * x + y * y + z * z < m_minUnloadedChunkDistance) {
                    m_unloadedChunks[i] = Position(x, y, z);
                    i++;
                }
            }
        }
    }
    std::sort(m_unloadedChunks, m_unloadedChunks + i,
    [](Position a, Position b) {
        return a.x * a.x + a.y * a.y + a.z * a.z < b.x * b.x + b.y * b.y + b.z * b.z;
    });
    m_nextUnloadedChunk = 0;
}

ServerPlayer::ServerPlayer(int playerID, int* blockPosition, float* subBlockPosition, unsigned short renderDistance, ENetPeer* peer) :
    m_renderDistance(renderDistance), m_renderDiameter(renderDistance * 2 + 1), m_playerID(playerID),  m_peer(peer) {
    m_blockPosition[0] = blockPosition[0];
    m_blockPosition[1] = blockPosition[1];
    m_blockPosition[2] = blockPosition[2];
    m_subBlockPosition[0] = subBlockPosition[0];
    m_subBlockPosition[1] = subBlockPosition[1];
    m_subBlockPosition[2] = subBlockPosition[2];
    m_playerChunkPosition[0] = std::floor((float)blockPosition[0] / constants::CHUNK_SIZE);
    m_playerChunkPosition[1] = std::floor((float)blockPosition[1] / constants::CHUNK_SIZE);
    m_playerChunkPosition[2] = std::floor((float)blockPosition[2] / constants::CHUNK_SIZE);
    m_playerChunkMovementOffset[0] = m_playerChunkMovementOffset[1] = m_playerChunkMovementOffset[2] = 0;
    initNumChunks();
    initChunkPositions();
}

ServerPlayer::ServerPlayer(int playerID, int* blockPosition, float* subBlockPosition, unsigned short renderDistance) :
    m_renderDistance(renderDistance), m_renderDiameter(renderDistance * 2 + 1), m_playerID(playerID) {
    m_blockPosition[0] = blockPosition[0];
    m_blockPosition[1] = blockPosition[1];
    m_blockPosition[2] = blockPosition[2];
    m_subBlockPosition[0] = subBlockPosition[0];
    m_subBlockPosition[1] = subBlockPosition[1];
    m_subBlockPosition[2] = subBlockPosition[2];
    m_playerChunkPosition[0] = std::floor((float)blockPosition[0] / constants::CHUNK_SIZE);
    m_playerChunkPosition[1] = std::floor((float)blockPosition[1] / constants::CHUNK_SIZE);
    m_playerChunkPosition[2] = std::floor((float)blockPosition[2] / constants::CHUNK_SIZE);
    m_playerChunkMovementOffset[0] = m_playerChunkMovementOffset[1] = m_playerChunkMovementOffset[2] = 0;
    initNumChunks();
    initChunkPositions();
}

void ServerPlayer::updatePlayerPos(int* blockPosition, float* subBlockPosition) {
    m_blockPosition[0] = blockPosition[0];
    m_blockPosition[1] = blockPosition[1];
    m_blockPosition[2] = blockPosition[2];
    m_subBlockPosition[0] = subBlockPosition[0];
    m_subBlockPosition[1] = subBlockPosition[1];
    m_subBlockPosition[2] = subBlockPosition[2];
    m_playerChunkMovementOffset[0] = std::floor((float)blockPosition[0] / constants::CHUNK_SIZE) - m_playerChunkPosition[0];
    m_playerChunkPosition[0] += m_playerChunkMovementOffset[0];
    m_playerChunkMovementOffset[1] = std::floor((float)blockPosition[1] / constants::CHUNK_SIZE) - m_playerChunkPosition[1];
    m_playerChunkPosition[1] += m_playerChunkMovementOffset[1];
    m_playerChunkMovementOffset[2] = std::floor((float)blockPosition[2] / constants::CHUNK_SIZE) - m_playerChunkPosition[2];
    m_playerChunkPosition[2] += m_playerChunkMovementOffset[2];
    m_processedChunk = m_loadedChunks.begin();
}

bool ServerPlayer::allChunksLoaded() {
    while ((m_nextUnloadedChunk < m_numChunks)
        && (m_loadedChunks.contains(m_unloadedChunks[m_nextUnloadedChunk] + m_playerChunkPosition))) {
        m_nextUnloadedChunk++;
    }
    return m_nextUnloadedChunk == m_numChunks;
}

void ServerPlayer::getNextChunkCoords(int* chunkCoords) {
    chunkCoords[0] = m_unloadedChunks[m_nextUnloadedChunk].x + m_playerChunkPosition[0];
    chunkCoords[1] = m_unloadedChunks[m_nextUnloadedChunk].y + m_playerChunkPosition[1];
    chunkCoords[2] = m_unloadedChunks[m_nextUnloadedChunk].z + m_playerChunkPosition[2];
    m_loadedChunks.emplace(chunkCoords);
    m_nextUnloadedChunk++;
}

bool ServerPlayer::decrementNextChunk(Position* chunkPosition, bool* chunkOutOfRange) {
    if (m_processedChunk == m_loadedChunks.end()) {
        m_nextUnloadedChunk = 0;
        *chunkOutOfRange = false;
        return false;
    }
    else {
        int a = m_processedChunk->x - m_playerChunkPosition[0];
        int b = m_processedChunk->y - m_playerChunkPosition[1];
        int c = m_processedChunk->z - m_playerChunkPosition[2];
        *chunkOutOfRange = a * a + b * b + c * c > m_minUnloadedChunkDistance - 0.001f;
        if (*chunkOutOfRange) {
            *chunkPosition = *m_processedChunk;
            m_processedChunk = m_loadedChunks.erase(m_processedChunk);
        }
        else {
            m_processedChunk++;
        }
        return true;
    }
}