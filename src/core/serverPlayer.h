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

#pragma once

#include "enet/enet.h"

#include "core/pch.h"

#include "core/chunk.h"
#include "core/position.h"

class ServerPlayer {
private:
    unsigned short m_renderDistance;
    unsigned short m_renderDiameter;
    float m_minUnloadedChunkDistance;
    unsigned int m_numChunks;  // The max number of chunks in the player's render distance
    int m_blockPosition[3];
    float m_subBlockPosition[3];
    Position* m_unloadedChunks;
    int m_playerChunkPosition[3];
    int m_playerChunkMovementOffset[3];
    int m_nextUnloadedChunk;
    int m_playerID;
    ENetPeer* m_peer;
    unsigned int m_lastPacketTick;
    std::unordered_set<Position> m_loadedChunks;
    std::unordered_set<Position>::iterator m_processedChunk;

    void initChunkPositions();
    void initNumChunks();
public:
    ServerPlayer() {};
    ServerPlayer(int playerID, int* blockPosition, float* subBlockPosition, unsigned short renderDistance, ENetPeer* peer, unsigned int gameTick);
    ServerPlayer(int playerID, int* blockPosition, float* subBlockPosition, unsigned short renderDistance);
    void updatePlayerPos(int* blockPosition, float* subBlockPosition);
    bool allChunksLoaded();
    void getNextChunkCoords(int* chunkCoords);
    bool decrementNextChunk(Position* chunkPosition, bool* chunkOutOfRange);
    
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

    inline bool hasChunkLoaded(Position& chunkPosition) {
        return m_loadedChunks.contains(chunkPosition);
    }

    inline void packetReceived(unsigned int gameTick) {
        m_lastPacketTick = gameTick;
    }

    inline unsigned int getLastPacketTick() const {
        return m_lastPacketTick;
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