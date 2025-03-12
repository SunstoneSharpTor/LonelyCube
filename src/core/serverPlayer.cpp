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

#include "core/serverPlayer.h"

#include "core/pch.h"

#include "core/chunk.h"
#include "core/constants.h"
#include "core/utils/iVec3.h"

namespace lonelycube {

void ServerPlayer::initChunks() {
    m_minUnloadedChunkDistance = (m_renderDistance + 1) * (m_renderDistance + 1);
    m_maxNumChunks = 0;
    for (int x = -m_renderDistance; x <= m_renderDistance; x++) {
        for (int y = -m_renderDistance; y <= m_renderDistance; y++) {
            for (int z = -m_renderDistance; z <= m_renderDistance; z++) {
                if (x * x + y * y + z * z < m_minUnloadedChunkDistance) {
                    m_maxNumChunks++;
                }
            }
        }
    }
    m_unloadedChunks = std::make_unique<IVec3[]>(m_maxNumChunks);
}

void ServerPlayer::initChunkPositions() {
    int i = 0;
    for (int x = -m_renderDistance; x <= m_renderDistance; x++) {
        for (int y = -m_renderDistance; y <= m_renderDistance; y++) {
            for (int z = -m_renderDistance; z <= m_renderDistance; z++) {
                if (x * x + y * y + z * z < m_minUnloadedChunkDistance) {
                    m_unloadedChunks[i] = IVec3(x, y, z);
                    i++;
                }
            }
        }
    }
    std::sort(&(m_unloadedChunks[0]), &(m_unloadedChunks[0]) + i,
        [](IVec3 a, IVec3 b) {
            int aDistance = a.x * a.x + a.y * a.y + a.z * a.z;
            int bDistance = b.x * b.x + b.y * b.y + b.z * b.z;
            if (aDistance == bDistance)
            {
                if (a.x == b.x)
                {
                    if (a.y == b.y)
                        return a.z < b.z;
                    else
                        return a.y < b.y;
                }
                else
                    return a.x < b.x;
            }
            else
                return aDistance < bDistance;
        }
    );
    m_nextUnloadedChunk = 0;
}

// The constructor used by the physical server
ServerPlayer::ServerPlayer(
    int playerID, int* blockPosition, float* subBlockPosition, uint16_t renderDistance,
    ENetPeer* peer, uint32_t gameTick
) : m_renderDistance(renderDistance), m_renderDiameter(renderDistance * 2 + 1),
    m_targetBufferSize(0), m_currentNumLoadedChunks(0), m_numChunkRequests(0), m_playerID(playerID),
    m_peer(peer), m_lastPacketTick(gameTick)
{
    m_blockPosition[0] = blockPosition[0];
    m_blockPosition[1] = blockPosition[1];
    m_blockPosition[2] = blockPosition[2];
    m_subBlockPosition[0] = subBlockPosition[0];
    m_subBlockPosition[1] = subBlockPosition[1];
    m_subBlockPosition[2] = subBlockPosition[2];
    IVec3 playerChunkPosition = Chunk::getChunkCoords(blockPosition);
    m_playerChunkPosition[0] = playerChunkPosition.x;
    m_playerChunkPosition[1] = playerChunkPosition.y;
    m_playerChunkPosition[2] = playerChunkPosition.z;
    initChunks();
    initChunkPositions();
}

// The constructor used by the integrated server
ServerPlayer::ServerPlayer(
    int playerID, int* blockPosition, float* subBlockPosition, uint16_t renderDistance,
    bool multiplayer
) : m_renderDistance(renderDistance), m_renderDiameter(renderDistance * 2 + 1),
    m_targetBufferSize(1), m_currentNumLoadedChunks(0), m_numChunkRequests(0), m_playerID(playerID)
{
    m_blockPosition[0] = blockPosition[0];
    m_blockPosition[1] = blockPosition[1];
    m_blockPosition[2] = blockPosition[2];
    m_subBlockPosition[0] = subBlockPosition[0];
    m_subBlockPosition[1] = subBlockPosition[1];
    m_subBlockPosition[2] = subBlockPosition[2];
    IVec3 playerChunkPosition = Chunk::getChunkCoords(blockPosition);
    m_playerChunkPosition[0] = playerChunkPosition.x;
    m_playerChunkPosition[1] = playerChunkPosition.y;
    m_playerChunkPosition[2] = playerChunkPosition.z;
    initChunks();
    initChunkPositions();
}

void ServerPlayer::updatePlayerPos(const IVec3& blockPosition, const Vec3& subBlockPosition) {
    m_blockPosition[0] = blockPosition[0];
    m_blockPosition[1] = blockPosition[1];
    m_blockPosition[2] = blockPosition[2];
    m_subBlockPosition[0] = subBlockPosition[0];
    m_subBlockPosition[1] = subBlockPosition[1];
    m_subBlockPosition[2] = subBlockPosition[2];
    IVec3 playerChunkCoords = Chunk::getChunkCoords(blockPosition);
    m_playerChunkPosition[0] = playerChunkCoords[0];
    m_playerChunkPosition[1] = playerChunkCoords[1];
    m_playerChunkPosition[2] = playerChunkCoords[2];
}

bool ServerPlayer::updateNextUnloadedChunk()
{
    while (
        (m_nextUnloadedChunk < m_maxNumChunks)
        && (m_loadedChunks.contains(m_unloadedChunks[m_nextUnloadedChunk] + m_playerChunkPosition)))
    {
        m_nextUnloadedChunk++;
    }
    return m_nextUnloadedChunk == m_maxNumChunks;
}

void ServerPlayer::getNextChunkCoords(int* chunkCoords)
{
    chunkCoords[0] = m_unloadedChunks[m_nextUnloadedChunk].x + m_playerChunkPosition[0];
    chunkCoords[1] = m_unloadedChunks[m_nextUnloadedChunk].y + m_playerChunkPosition[1];
    chunkCoords[2] = m_unloadedChunks[m_nextUnloadedChunk].z + m_playerChunkPosition[2];
    m_loadedChunks.emplace(chunkCoords);
    m_nextUnloadedChunk++;
}

void ServerPlayer::beginUnloadingChunks()
{
    m_processedChunk = m_loadedChunks.begin();
}

bool ServerPlayer::checkIfNextChunkShouldUnload(IVec3* chunkPosition, bool* chunkOutOfRange)
{
    if (m_processedChunk == m_loadedChunks.end()) {
        m_nextUnloadedChunk = 0;
        m_currentNumLoadedChunks = 0;
        *chunkOutOfRange = false;
        return false;
    }

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

bool ServerPlayer::updateChunkLoadingTarget()
{
    updateNextUnloadedChunk();
    int oldTarget = m_currentNumLoadedChunks;
    m_currentNumLoadedChunks = m_nextUnloadedChunk;
    return m_currentNumLoadedChunks != oldTarget;
}

}  // namespace lonelycube
