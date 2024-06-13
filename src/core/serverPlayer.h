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

#include <unordered_set>

#include "core/chunk.h"
#include "core/position.h"

class ServerPlayer {
private:
    unsigned short m_renderDistance;
    unsigned short m_renderDiameter;
    float m_minUnloadedChunkDistance;
    unsigned int m_numChunks;
    int m_blockPosition[3];
    float m_subBlockPosition[3];
    Position* m_unloadedChunks;
    int* m_playerChunkPosition;
    int m_nextUnloadedChunk;
    int m_playerID;
    std::unordered_set<Position> m_loadedChunks;

    void initChunkPositions();
    void initNumChunks();
public:
    ServerPlayer(int playerID, int* blockPosition, float* subBlockPosition, unsigned short renderDistance);
    void updatePlayerPos(int* blockPosition, float* subBlockPosition);
    bool allChunksLoaded();
    void getNextChunkCoords(int* chunkCoords);
    bool decrementNextChunk(Position* chunkPosition, bool* chunkOutOfRange);
    bool hasChunkLoaded(Position& chunkPosition);
    
    inline int getID() const {
        return m_playerID;
    }

    inline void getChunkPosition(int* position) {
        position[0] = m_playerChunkPosition[0];
        position[1] = m_playerChunkPosition[1];
        position[2] = m_playerChunkPosition[2];
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