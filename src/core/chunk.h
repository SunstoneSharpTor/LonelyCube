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

#include "core/constants.h"
#include "core/position.h"

class Chunk {
private:
    unsigned char* m_blocks;
    unsigned char* m_skyLight;
    bool m_singleBlockType;
    bool m_singleSkyLightVal;
    bool m_skyLightUpToDate;
    unsigned short m_playerCount; // The number of players that are rendering this chunk
    int m_position[3]; //the chunks position in chunk coordinates (multiply by chunk size to get world coordinates)
    bool m_calculatingSkylight;
    std::unordered_map<Position, Chunk>* m_worldChunks;
    //std::mutex m_accessingSkylightMtx;
    //std::condition_variable m_accessingSkylightCV;

    static const short m_neighbouringBlocksX[6];
    static const short m_neighbouringBlocksY[6];
    static const short m_neighbouringBlocksZ[6];
    static const float m_cubeTextureCoordinates[48];
    static const float m_xTextureCoordinates[32];
    static const short m_blockIdToTextureNum[48];
    static const int m_neighbouringChunkBlockOffsets[6];
    static const short m_adjacentBlocksToFaceOffests[48];
    static const short m_adjacentBlocksToFaceOffestsX[48];
    static const short m_adjacentBlocksToFaceOffestsY[48];
    static const short m_adjacentBlocksToFaceOffestsZ[48];

    inline void findBlockCoordsInWorld(int* blockPos, unsigned int block) {
        int chunkCoords[3];
        getChunkPosition(chunkCoords);
        blockPos[0] = block % constants::CHUNK_SIZE + chunkCoords[0] * constants::CHUNK_SIZE;
        blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE) + chunkCoords[1] * constants::CHUNK_SIZE;
        blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE + chunkCoords[2] * constants::CHUNK_SIZE;
    }

    inline static void findBlockCoordsInChunk(float* blockPos, unsigned int block) {
        blockPos[0] = block % constants::CHUNK_SIZE;
        blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE;
    }

    inline static void findBlockCoordsInChunk(unsigned short* blockPos, unsigned int block) {
        blockPos[0] = block % constants::CHUNK_SIZE;
        blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE;
    }

    void addFaceToMesh(float* vertices, unsigned int* numVertices, unsigned int* indices, unsigned int* numIndices, float* waterVertices, unsigned int* numWaterVertices, unsigned int* waterIndices, unsigned int* numWaterIndices, unsigned int block, unsigned char neighbouringBlock);

    void getTextureCoordinates(float* coords, short textureNum);

public:
    static std::mutex s_checkingNeighbouringRelights;
    
    bool inUse;

    static const short neighbouringBlocks[6];

    inline static unsigned int getBlockNumber(unsigned int* blockCoords) {
        return blockCoords[0] + blockCoords[1] * constants::CHUNK_SIZE * constants::CHUNK_SIZE + blockCoords[2] * constants::CHUNK_SIZE;
    }

    Chunk(Position position, std::unordered_map<Position, Chunk>* worldChunks);

    Chunk();

    void getChunkPosition(int* coordinates) const;

    void unload();

    inline char getBlock(const unsigned int block) const {
        return m_blocks[block * (!m_singleBlockType)];
    }

    inline char getBlockUnchecked(unsigned int block) {
        return m_blocks[block];
    }

    void setBlock(unsigned int block, unsigned char blockType);

    inline void setBlockUnchecked(unsigned int block, unsigned char blockType) {
        m_blocks[block] = blockType;
    }

    inline char getSkyLight(const unsigned int block) const {
        return (m_skyLight[block / 2] >> (4 * (block % 2))) & 0b1111;
    }
    
    inline void setSkyLight(unsigned int block, unsigned char value) {
        bool oddBlockNum = block % 2;
        bool evenBlockNum = !oddBlockNum;
        m_skyLight[block / 2] &= 0b00001111 << (4 * evenBlockNum);
        m_skyLight[block / 2] |= value << (4 * oddBlockNum);
    }

    void clearBlocksAndLight();

    unsigned char getWorldBlock(int* blockCoords);

    unsigned char getWorldSkyLight(int* blockCoords);

    inline void setSkyLightToBeOutdated() {
        m_skyLightUpToDate = false;
    }

    inline void setSkyLightToBeUpToDate() {
        m_skyLightUpToDate = true;
    }

    inline bool isSkylightUpToDate() {
        return m_skyLightUpToDate;
    }

    inline bool isSkyBeingRelit() {
        return m_calculatingSkylight;
    }

    inline void setSkylightBeingRelit(bool val) {
        m_calculatingSkylight = val;
    }

    inline void incrementPlayerCount() {
        m_playerCount++;
    }

    inline void decrementPlayerCount() {
        m_playerCount--;
    }

    inline bool hasNoPlayers() {
        return m_playerCount == 0;
    }

    inline unsigned short getPlayerCount() const {
        return m_playerCount;
    }

    inline bool isSingleBlockType() {
        return m_singleBlockType;
    }

    inline void setSingleBlockType(bool val) {
        m_singleBlockType = val;
    }
    
    void clearSkyLight();

    void uncompressBlocks();

    void compressBlocks();
};