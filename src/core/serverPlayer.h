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

#include "enet/enet.h"

#include "core/pch.h"

#include "core/constants.h"
#include "core/log.h"
#include "core/utils/iVec3.h"

namespace lonelycube {

class ServerPlayer {
private:
    int m_renderDistance;
    int m_renderDiameter;
    float m_minUnloadedChunkDistance;
    uint32_t m_maxNumChunks;  // The max number of chunks in the player's render distance
    int m_blockPosition[3];
    float m_subBlockPosition[3];
    std::unique_ptr<IVec3[]> m_unloadedChunks;
    int m_playerChunkPosition[3];
    int m_nextUnloadedChunk;
    int m_targetBufferSize;
    int m_currentNumLoadedChunks;
    int64_t m_numChunkRequests;
    uint32_t m_playerID;
    ENetPeer* m_peer;
    uint32_t m_lastPacketTick;
    std::map<IVec3, uint64_t> m_loadedChunks;
    std::map<IVec3, uint64_t>::iterator m_processedChunk;

    void initChunkPositions();
    void initChunks();

public:
    ServerPlayer() {};
    ServerPlayer(
        uint32_t playerID, int* blockPosition, float* subBlockPosition, int renderDistance,
        ENetPeer* peer, uint32_t gameTick
    );
    ServerPlayer(
        uint32_t playerID, int* blockPosition, float* subBlockPosition, int renderDistance,
        bool multiplayer
    );
    void updatePlayerPos(const IVec3& blockPosition, const Vec3& subBlockPosition);
    bool updateNextUnloadedChunk();
    void getNextChunkCoords(int* chunkCoords, uint64_t currentGameTick);
    void beginUnloadingChunks();
    bool checkIfNextChunkShouldUnload(IVec3* chunkPosition, bool* chunkOutOfRange);
    bool updateChunkLoadingTarget();

    inline void setChunkLoaded(const IVec3& chunkPosition, uint64_t currentGameTick)
    {
        m_loadedChunks[chunkPosition] = currentGameTick;
    }

    inline uint32_t getID() const {
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

    inline int getRenderDistance() const {
        return m_renderDistance;
    }

    inline void getBlockPosition(int* blockPosition) {
        blockPosition[0] = m_blockPosition[0];
        blockPosition[1] = m_blockPosition[1];
        blockPosition[2] = m_blockPosition[2];
    }
    inline void setChunkLoadingTarget(int target, uint64_t currentTickNum)
    {
        m_currentNumLoadedChunks = target;

        // If the server thinks the target chunk was already sent to the client
        // a long time ago, resend it because the client has probably unloaded it
        auto it = m_loadedChunks.find(m_unloadedChunks[target] + m_playerChunkPosition);
        if (it != m_loadedChunks.end() && it->second + constants::TICKS_PER_SECOND < currentTickNum)
        {
            LOG("Resending " + std::to_string(target));
            m_loadedChunks.erase(it);
            m_nextUnloadedChunk = target;
        }
    }
    inline int getChunkLoadingTarget() const
    {
        return m_currentNumLoadedChunks;
    }
    inline bool wantsMoreChunks()
    {
        return m_nextUnloadedChunk < m_currentNumLoadedChunks + m_targetBufferSize;
    }
    inline int getTargetBufferSize() const
    {
        return m_targetBufferSize;
    }
    inline void setTargetBufferSize(const int bufferSize)
    {
        m_targetBufferSize = bufferSize;
    }
    inline int64_t getNumChunkRequests()
    {
        return m_numChunkRequests;
    }
    inline void setNumChunkRequests(int64_t numRequests)
    {
        m_numChunkRequests = numRequests;
    }
    inline int64_t incrementNumChunkRequests()
    {
        return ++m_numChunkRequests;
    }
};

}  // namespace lonelycube

namespace std {
    template<>
    struct hash<lonelycube::ServerPlayer> {
        size_t operator()(const lonelycube::ServerPlayer& key) const {
            return key.getID();
        }
    };
}
