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

#include "core/pch.h"

#include "enet/enet.h"

#include "core/chunk.h"
#include "core/packet.h"
#include "core/serverPlayer.h"

class ServerWorld {
private:
    bool m_singleplayer;
    bool m_integrated;
	unsigned long long m_seed;
    unsigned short m_nextPlayerID;
    unsigned short m_numChunkLoadingThreads;
    unsigned int m_gameTick;
    
    std::unordered_map<Position, Chunk> m_chunks;
    std::unordered_map<unsigned short, ServerPlayer> m_players;
    std::queue<Position> m_chunksToBeLoaded;
    std::unordered_set<Position> m_chunksBeingLoaded;

    // Synchronisation
    std::mutex m_chunksMtx;
    std::mutex m_playersMtx;
    std::mutex m_chunksToBeLoadedMtx;
    std::mutex m_chunksBeingLoadedMtx;
    std::mutex m_unmeshedChunksMtx;
    std::mutex m_threadsWaitMtx;
    std::condition_variable m_threadsWaitCV;
    bool m_threadsWait;
	bool* m_threadWaiting;

    void disconnectPlayer(unsigned short playerID);

public:
    ServerWorld(bool singleplayer, bool integrated, unsigned long long seed);
    void tick();
    void addPlayer(int* blockPosition, float* subBlockPosition, unsigned short renderDistance);
    unsigned short addPlayer(int* blockPosition, float* subBlockPosition, unsigned short renderDistance, ENetPeer* peer);
    void updatePlayerPos(int playerID, int* blockPosition, float* subBlockPosition, bool unloadNeeded);
    ServerPlayer& getPlayer(int playerID) {
        return m_players.at(playerID);
    }
    std::unordered_map<unsigned short, ServerPlayer>& getPlayers() {
        return m_players;
    }
    void waitIfRequired(unsigned char threadNum);
    void findChunksToLoad();
    bool loadChunk(Position* chunkPosition);
    void loadChunkFromPacket(Packet<unsigned char, 9 * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE * constants::CHUNK_SIZE>& payload, Position& chunkPosition);
    bool getNextLoadedChunkPosition(Position* chunkPosition);
    unsigned char getBlock(const Position& position) const;
    void setBlock(const Position& position, unsigned char blockType);
    unsigned char getSkyLight(const Position& position) const;
    inline Chunk& getChunk(const Position& chunkPosition) {
        Chunk& chunk = m_chunks.at(chunkPosition);
        return chunk;
    }
    inline bool chunkLoaded(const Position& chunkPosition) {
        return m_chunks.contains(chunkPosition);
    }
    inline std::unordered_map<Position, Chunk>& getWorldChunks() {
        return m_chunks;
    }
	inline char getNumChunkLoaderThreads() {
		return m_numChunkLoadingThreads;
	}
    inline unsigned int getTickNum() {
        return m_gameTick;
    }
};