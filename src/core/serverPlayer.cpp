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
#include "core/utils/iVec3.h"

namespace lonelycube {

void ServerPlayer::calculateMaxNumChunks() {
    m_maxLoadedChunkDistance = (m_renderDistance + 1) * (m_renderDistance + 1);
    m_maxNumChunks = 0;
    for (int x = -m_renderDistance; x <= m_renderDistance; x++) {
        for (int y = -m_renderDistance; y <= m_renderDistance; y++) {
            for (int z = -m_renderDistance; z <= m_renderDistance; z++) {
                if (x * x + y * y + z * z < m_maxLoadedChunkDistance) {
                    m_maxNumChunks++;
                }
            }
        }
    }
    m_chunkLoadingOrder.reserve(m_maxNumChunks);
}

void ServerPlayer::initChunkLoadingOrder() {
    for (int x = -m_renderDistance; x <= m_renderDistance; x++) {
        for (int y = -m_renderDistance; y <= m_renderDistance; y++) {
            for (int z = -m_renderDistance; z <= m_renderDistance; z++) {
                if (x * x + y * y + z * z < m_maxLoadedChunkDistance) {
                    m_chunkLoadingOrder.emplace_back(x, y, z);
                }
            }
        }
    }
    std::sort(m_chunkLoadingOrder.begin(), m_chunkLoadingOrder.end(),
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
    for (const auto& pos : m_chunkLoadingOrder)
        m_chunkDistances.push_back(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);

    m_nextUnloadedChunk = 0;
}

// The constructor used by the physical server
ServerPlayer::ServerPlayer(
    uint32_t playerID, int* blockPos, float* subBlockPos, int renderDistance, ENetPeer* peer,
    uint64_t gameTick
) : m_renderDistance(renderDistance), m_renderDiameter(renderDistance * 2 + 1),
    m_targetBufferSize(0), m_currentNumLoadedChunks(0), m_numChunkRequests(0), m_playerID(playerID),
    m_peer(peer), m_lastPacketTick(gameTick)
{
    m_blockPos[0] = blockPos[0];
    m_blockPos[1] = blockPos[1];
    m_blockPos[2] = blockPos[2];
    m_subBlockPos[0] = subBlockPos[0];
    m_subBlockPos[1] = subBlockPos[1];
    m_subBlockPos[2] = subBlockPos[2];
    IVec3 playerChunkPos= Chunk::getChunkCoords(blockPos);
    m_playerChunkPos[0] = playerChunkPos.x;
    m_playerChunkPos[1] = playerChunkPos.y;
    m_playerChunkPos[2] = playerChunkPos.z;
    calculateMaxNumChunks();
    initChunkLoadingOrder();
}

// The constructor used by the integrated server
ServerPlayer::ServerPlayer(
    uint32_t playerID, int* blockPos, float* subBlockPos, int renderDistance, bool multiplayer
) : m_renderDistance(renderDistance), m_renderDiameter(renderDistance * 2 + 1),
    m_targetBufferSize(90), m_currentNumLoadedChunks(0), m_numChunkRequests(0), m_playerID(playerID)
{
    m_blockPos[0] = blockPos[0];
    m_blockPos[1] = blockPos[1];
    m_blockPos[2] = blockPos[2];
    m_subBlockPos[0] = subBlockPos[0];
    m_subBlockPos[1] = subBlockPos[1];
    m_subBlockPos[2] = subBlockPos[2];
    IVec3 playerChunkPos= Chunk::getChunkCoords(blockPos);
    m_playerChunkPos[0] = playerChunkPos.x;
    m_playerChunkPos[1] = playerChunkPos.y;
    m_playerChunkPos[2] = playerChunkPos.z;
    calculateMaxNumChunks();
    initChunkLoadingOrder();
}

void ServerPlayer::updatePlayerPos(const IVec3& blockPos, const Vec3& subBlockPos) {
    m_blockPos[0] = blockPos[0];
    m_blockPos[1] = blockPos[1];
    m_blockPos[2] = blockPos[2];
    m_subBlockPos[0] = subBlockPos[0];
    m_subBlockPos[1] = subBlockPos[1];
    m_subBlockPos[2] = subBlockPos[2];
    IVec3 playerChunkPos = Chunk::getChunkCoords(blockPos);
    m_playerChunkPos[0] = playerChunkPos[0];
    m_playerChunkPos[1] = playerChunkPos[1];
    m_playerChunkPos[2] = playerChunkPos[2];
}

bool ServerPlayer::updateNextUnloadedChunk()
{
    while (
        (m_nextUnloadedChunk < m_maxNumChunks)
        && (m_loadedChunks.contains(m_chunkLoadingOrder[m_nextUnloadedChunk] + m_playerChunkPos)))
    {
        m_nextUnloadedChunk++;
    }
    return m_nextUnloadedChunk < m_maxNumChunks;
}

void ServerPlayer::getNextChunkCoords(int* chunkCoords, uint64_t currentGameTick)
{
    chunkCoords[0] = m_chunkLoadingOrder[m_nextUnloadedChunk].x + m_playerChunkPos[0];
    chunkCoords[1] = m_chunkLoadingOrder[m_nextUnloadedChunk].y + m_playerChunkPos[1];
    chunkCoords[2] = m_chunkLoadingOrder[m_nextUnloadedChunk].z + m_playerChunkPos[2];
    m_loadedChunks[chunkCoords] = currentGameTick;
    m_nextUnloadedChunk++;
}

void ServerPlayer::beginUnloadingChunksOutOfRange()
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

    int a = m_processedChunk->first.x - m_playerChunkPos[0];
    int b = m_processedChunk->first.y - m_playerChunkPos[1];
    int c = m_processedChunk->first.z - m_playerChunkPos[2];
    *chunkOutOfRange = a * a + b * b + c * c >= m_maxLoadedChunkDistance;
    if (*chunkOutOfRange) {
        *chunkPosition = m_processedChunk->first;
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

void ServerPlayer::setChunkLoadingTarget(int target, uint64_t currentTickNum)
{
    m_currentNumLoadedChunks = target;
    if (target >= m_maxNumChunks)
    {
        target = m_maxNumChunks - 1;
        return;
    }

    // If the server thinks the target chunk was already sent to the client
    // a long time ago, resend it because the client has probably unloaded it
    auto it = m_loadedChunks.find(m_chunkLoadingOrder[target] + m_playerChunkPos);
    if (it != m_loadedChunks.end() && it->second + constants::TICKS_PER_SECOND < currentTickNum)
    {
        LOG("Resending " + std::to_string(target));
        m_loadedChunks.erase(it);
        m_nextUnloadedChunk = target;
    }
}

}  // namespace lonelycube
