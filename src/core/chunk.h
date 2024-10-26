/*
  Lonely Cube, a voxel game
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
    std::array<unsigned char*, constants::CHUNK_SIZE> m_blocks;
    std::array<short, constants::CHUNK_SIZE> m_layerBlockTypes;
    std::array<unsigned char*, constants::CHUNK_SIZE> m_skyLight;
    std::array<unsigned char, constants::CHUNK_SIZE> m_layerSkyLightValues;
    bool m_skyLightUpToDate;
    unsigned short m_playerCount; // The number of players that are rendering this chunk
    int m_position[3]; //the chunks position in chunk coordinates (multiply by chunk size to get world coordinates)
    bool m_calculatingSkylight;
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

    void getTextureCoordinates(float* coords, short textureNum);

public:
    static std::mutex s_checkingNeighbouringRelights;

    static const short neighbouringBlocks[6];

    inline static unsigned int getBlockNumber(unsigned int* blockCoords) {
        return blockCoords[0] + blockCoords[1] * constants::CHUNK_SIZE * constants::CHUNK_SIZE +
            blockCoords[2] * constants::CHUNK_SIZE;
    }

    Chunk(Position position);

    Chunk();

    void getChunkPosition(int* coordinates) const;

    void unload();

    inline unsigned char getBlock(const unsigned int block) const {
        unsigned int layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        if (m_layerBlockTypes[layerNum] == 256) {
            return m_blocks[layerNum][block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)];
        }
        return m_layerBlockTypes[layerNum];
    }

    inline unsigned char getBlockUnchecked(unsigned int block) {
        unsigned int layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        return m_blocks[layerNum][block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)];
    }

    void setBlock(unsigned int block, unsigned char blockType);

    inline void setBlockUnchecked(unsigned int block, unsigned char blockType) {
        unsigned int layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        m_blocks[layerNum][block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)] = blockType;
    }

    inline unsigned char getSkyLight(const unsigned int block) const {
        unsigned int layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        if (m_layerSkyLightValues[layerNum] == constants::skyLightMaxValue + 1) {
            return (m_skyLight[layerNum][block % (constants::CHUNK_SIZE *
                constants::CHUNK_SIZE) / 2] >> (4 * (block % 2))) & 0b1111;
        }
        return m_layerSkyLightValues[layerNum];
    }
    
    void setSkyLight(unsigned int block, unsigned char value);

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

    inline unsigned short getLayerBlockType(unsigned int layerNum) {
        return m_layerBlockTypes[layerNum];
    }
    
    void clearSkyLight();

    void clearBlocksAndLight();

    void compressBlocksAndLight();

    void compressSkyLight();

    void uncompressBlocksAndLight();
};