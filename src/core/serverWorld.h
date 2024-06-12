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

#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "core/chunk.h"
#include "core/serverPlayer.h"

class ServerWorld {
private:
    bool m_multiplayer;
	unsigned long long m_seed;
    int m_nextPlayerID;
    unsigned short m_numChunkLoadingThreads;
    
    std::unordered_map<Position, Chunk> m_chunks;
    std::unordered_map<int, ServerPlayer> m_players;
    std::queue<Position> m_chunksToBeLoaded;
    std::unordered_set<Position> m_chunksBeingLoaded;

    void loadChunks();
public:
    ServerWorld(bool multiplayer, unsigned long long seed);
    void updatePlayerPos(int playerID, int* blockPosition, float* subBlockPosition);
};