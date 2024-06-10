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

#include <algorithm>
#include <cmath>
#include <iostream>

#include "core/chunk.h"
#include "core/constants.h"

void ServerPlayer::initNumChunks() {
    m_minUnloadedChunkDistance = ((m_renderDistance + 1) * (m_renderDistance + 1));
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
                m_unloadedChunks[i] = Position(x, y, z);
                i++;
            }
        }
    }
    std::sort(m_unloadedChunks, m_unloadedChunks + i,
    [](Position a, Position b) {
        return a.x * a.x + a.y * a.y + a.z * a.z < b.x * b.x + b.y * b.y + b.z * b.z;
    });
    m_nextUnloadedChunk = 0;
}

ServerPlayer::ServerPlayer(double* position, unsigned short renderDistance) :
    m_renderDistance(renderDistance), m_renderDiameter(renderDistance * 2 + 1) {
    m_position[0] = position[0];
    m_position[1] = position[1];
    m_position[2] = position[2];
    m_playerChunkPosition[0] = floor(position[0] / static_cast<float>(constants::CHUNK_SIZE));
    m_playerChunkPosition[1] = floor(position[1] / static_cast<float>(constants::CHUNK_SIZE));
    m_playerChunkPosition[2] = floor(position[2] / static_cast<float>(constants::CHUNK_SIZE));
    initNumChunks();
    initChunkPositions();
}

void ServerPlayer::updatePlayerPos(double* position) {
    m_playerChunkPosition[0] = floor(position[0] / static_cast<float>(constants::CHUNK_SIZE));
    m_playerChunkPosition[1] = floor(position[1] / static_cast<float>(constants::CHUNK_SIZE));
    m_playerChunkPosition[2] = floor(position[2] / static_cast<float>(constants::CHUNK_SIZE));
    // If the player has moved chunk, remove all the chunks that are out of
    // render distance from the set of loaded chunks
    if (m_playerChunkPosition[0] != m_playerChunkPosition[0]
        || m_playerChunkPosition[1] != m_playerChunkPosition[1]
        || m_playerChunkPosition[2] != m_playerChunkPosition[2]) {
        while (m_nextUnloadedChunk > 0) {
            m_nextUnloadedChunk--;
            int a = m_unloadedChunks[m_nextUnloadedChunk].x - m_playerChunkPosition[0];
            int b = m_unloadedChunks[m_nextUnloadedChunk].y - m_playerChunkPosition[1];
            int c = m_unloadedChunks[m_nextUnloadedChunk].z - m_playerChunkPosition[2];
            if (a * a + b * b + c * c > m_minUnloadedChunkDistance - 0.001f) {
                m_loadedChunks.erase(m_unloadedChunks[m_nextUnloadedChunk]);
            }
        }
    }
}

bool ServerPlayer::allChunksLoaded() {
    while ((m_nextUnloadedChunk < m_numChunks)
        && (m_loadedChunks.count(m_unloadedChunks[m_numChunks]))) {
        m_numChunks++;
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