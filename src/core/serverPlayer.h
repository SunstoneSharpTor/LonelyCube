/*
  Lonely Cube, a voxel game
  Copyright (C) g 2024-2025 Bertie Cartwright

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

#include "enet/enet.h"

#include "core/pch.h"

#include "core/chunk.h"
#include "core/utils/iVec3.h"

class ServerPlayer {
private:
    uint16_t m_renderDistance;
    uint16_t m_renderDiameter;
    float m_minUnloadedChunkDistance;
    uint32_t m_maxNumChunks;  // The max number of chunks in the player's render distance
    int m_blockPosition[3];
    float m_subBlockPosition[3];
    std::unique_ptr<IVec3[]> m_unloadedChunks;
    int m_playerChunkPosition[3];
    int m_nextUnloadedChunk;
    int m_targetBufferSize;
    int m_targetNumLoadedChunks;
    int64_t m_numChunkRequests;
    int m_playerID;
    ENetPeer* m_peer;
    uint32_t m_lastPacketTick;
    std::unordered_set<IVec3> m_loadedChunks;
    std::unordered_set<IVec3>::iterator m_processedChunk;

    void initChunkPositions();
    void initChunks();
public:
    ServerPlayer() {};
    ServerPlayer(int playerID, int* blockPosition, float* subBlockPosition, uint16_t renderDistance, ENetPeer* peer, uint32_t gameTick);
    ServerPlayer(int playerID, int* blockPosition, float* subBlockPosition, uint16_t renderDistance, bool multiplayer);
    void updatePlayerPos(const IVec3& blockPosition, const Vec3& subBlockPosition);
    bool updateNextUnloadedChunk();
    void getNextChunkCoords(int* chunkCoords);
    bool checkIfNextChunkShouldUnload(IVec3* chunkPosition, bool* chunkOutOfRange);
    bool updateChunkLoadingTarget();

    inline void setChunkLoaded(const IVec3& chunkPosition)
    {
        m_loadedChunks.insert(chunkPosition);
    }

    inline int getID() const {
        return m_playerID;
    }

    inline ENetPeer* getPeer() const {
        return m_peer;
    }

    inline void getChunkPosition(int* position) {
        position[0] = m_playerChunkPosition[0];
        position[1] = m_playerChunkPosition[1];
        position[2] = m_playerChunkPosition[2];
    }

    inline IVec3 getChunkPosition()
    {
        return { m_playerChunkPosition[0], m_playerChunkPosition[1], m_playerChunkPosition[2] };
    }

    inline bool hasChunkLoaded(const IVec3& chunkPosition) {
        return m_loadedChunks.contains(chunkPosition);
    }

    inline void packetReceived(uint32_t gameTick) {
        m_lastPacketTick = gameTick;
    }

    inline uint32_t getLastPacketTick() const {
        return m_lastPacketTick;
    }

    inline uint16_t getRenderDistance() const {
        return m_renderDistance;
    }

    inline void getBlockPosition(int* blockPosition) {
        blockPosition[0] = m_blockPosition[0];
        blockPosition[1] = m_blockPosition[1];
        blockPosition[2] = m_blockPosition[2];
    }
    inline void setChunkLoadingTarget(int target)
    {
        m_targetNumLoadedChunks = target;
    }
    inline int getChunkLoadingTarget() const
    {
        return m_targetNumLoadedChunks;
    }
    inline bool wantsMoreChunks()
    {
        return m_nextUnloadedChunk < m_targetNumLoadedChunks;
    }
    inline int64_t getNumChunkRequests()
    {
        return m_numChunkRequests;
    }
    inline int incrementNumChunkRequests()
    {
        return ++m_numChunkRequests;
    }
    int getChunkNumber(const IVec3& chunkCoords)
    {
        IVec3 relativeChunkCoords = chunkCoords - m_playerChunkPosition;
        int i;
        for (i = 0; m_unloadedChunks[i] != relativeChunkCoords; i++) {}
        return i;
    }
};

namespace std {
    template<>
    struct hash<ServerPlayer> {
        size_t operator()(const ServerPlayer& key) const {
            return key.getID();
        }
    };
}
