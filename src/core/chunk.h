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

#include "core/pch.h"

#include "core/constants.h"
#include "core/utils/iVec3.h"
#include <atomic>

namespace lonelycube {

class Chunk {
private:
    std::array<uint8_t*, constants::CHUNK_SIZE> m_blocks;
    std::array<int32_t, constants::CHUNK_SIZE> m_layerBlockTypes;
    std::array<uint8_t*, constants::CHUNK_SIZE> m_skyLight;
    std::array<uint32_t, constants::CHUNK_SIZE> m_layerSkyLightValues;
    std::array<uint8_t*, constants::CHUNK_SIZE> m_blockLight;
    std::array<uint32_t, constants::CHUNK_SIZE> m_layerBlockLightValues;
    bool m_skyLightUpToDate;
    bool m_blockLightUpToDate;
    uint16_t m_playerCount;  // The number of players that are rendering this chunk
    IVec3 m_position;  // The position in chunk coordinates (multiply by chunk size to get world coordinates)
    bool m_skyLightBeingRelit;
    bool m_blockLightBeingRelit;

    inline void findBlockCoordsInWorld(int* blockPos, uint32_t block) {
        int chunkCoords[3];
        getPosition(chunkCoords);
        blockPos[0] = block % constants::CHUNK_SIZE + chunkCoords[0] * constants::CHUNK_SIZE;
        blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE) + chunkCoords[1] * constants::CHUNK_SIZE;
        blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE + chunkCoords[2] * constants::CHUNK_SIZE;
    }

    inline static void findBlockCoordsInChunk(int* blockPos, uint32_t block) {
        blockPos[0] = block % constants::CHUNK_SIZE;
        blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE;
    }

    inline static void findBlockCoordsInChunk(uint16_t* blockPos, uint32_t block) {
        blockPos[0] = block % constants::CHUNK_SIZE;
        blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE;
    }

public:
    static std::mutex s_checkingNeighbourSkyRelightsMtx;
    static std::mutex s_checkingNeighbourBlockRelightsMtx;

    static constexpr int16_t neighbouringBlocks[6] = {
        -(constants::CHUNK_SIZE * constants::CHUNK_SIZE),
        -constants::CHUNK_SIZE,
        -1,
        1,
        constants::CHUNK_SIZE,
        (constants::CHUNK_SIZE * constants::CHUNK_SIZE)
    };

    inline static uint32_t getBlockNumber(uint32_t* blockCoords) {
        return blockCoords[0] + blockCoords[1] * constants::CHUNK_SIZE * constants::CHUNK_SIZE +
            blockCoords[2] * constants::CHUNK_SIZE;
    }

    inline static IVec3 getChunkCoords(const IVec3& blockCoords)
    {
        IVec3 chunkCoords {
            blockCoords.x >= 0 ? blockCoords.x / constants::CHUNK_SIZE :
                (blockCoords.x + 1) / constants::CHUNK_SIZE - 1,
            blockCoords.y >= 0 ? blockCoords.y / constants::CHUNK_SIZE :
                (blockCoords.y + 1) / constants::CHUNK_SIZE - 1,
            blockCoords.z >= 0 ? blockCoords.z / constants::CHUNK_SIZE :
                (blockCoords.z + 1) / constants::CHUNK_SIZE - 1 };
        return chunkCoords;
    }

    Chunk(IVec3 position);

    Chunk();

    void getPosition(int* coordinates) const;

    void unload();

    inline uint32_t getBlock(const uint32_t block) const {
        uint32_t layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        if (m_layerBlockTypes[layerNum] == 256) {
            return m_blocks[layerNum][block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)];
        }
        return m_layerBlockTypes[layerNum];
    }

    inline uint32_t getBlockUnchecked(uint32_t block) {
        uint32_t layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        return m_blocks[layerNum][block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)];
    }

    void setBlock(uint32_t block, uint32_t blockType);

    inline void setBlockUnchecked(uint32_t block, uint32_t blockType) {
        uint32_t layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        m_blocks[layerNum][block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)] = blockType;
    }

    inline uint32_t getSkyLight(const uint32_t block) const {
        uint32_t layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        if (m_layerSkyLightValues[layerNum] == constants::skyLightMaxValue + 1) {
            std::size_t blockNumInLayer = block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
            std::size_t index = blockNumInLayer * 5 / 8;
            std::size_t offset = blockNumInLayer * 5 % 8;
            uint32_t firstByte = m_skyLight[layerNum][index];
            uint32_t secondByte =  m_skyLight[layerNum][index + 1];
            return ((firstByte + (secondByte << 8)) >> offset) & 0b11111;
        }
        return m_layerSkyLightValues[layerNum];
    }

    inline uint32_t getBlockLight(const uint32_t block) const {
        uint32_t layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        if (m_layerBlockLightValues[layerNum] == constants::blockLightMaxValue + 1) {
            return (m_blockLight[layerNum][block % (constants::CHUNK_SIZE *
                constants::CHUNK_SIZE) / 2] >> (4 * (block % 2))) & 0b1111;
        }
        return m_layerBlockLightValues[layerNum];
    }

    void setSkyLight(const uint32_t block, const uint32_t value);

    void setBlockLight(const uint32_t block, const uint32_t value);

    inline void setSkyLightToBeOutdated() {
        m_skyLightUpToDate = false;
    }

    inline void setBlockLightToBeOutdated() {
        m_blockLightUpToDate = false;
    }

    inline void setSkyLightToBeUpToDate() {
        m_skyLightUpToDate = true;
    }

    inline void setBlockLightToBeUpToDate() {
        m_blockLightUpToDate = true;
    }

    inline bool isSkyLightUpToDate() {
        return m_skyLightUpToDate;
    }

    inline bool isBlockLightUpToDate() {
        return m_blockLightUpToDate;
    }

    inline bool isSkyLightBeingRelit() {
        return m_skyLightBeingRelit;
    }

    inline void setSkyLightBeingRelit(bool val) {
        m_skyLightBeingRelit = val;
    }

    inline bool isBlockLightBeingRelit() {
        return m_blockLightBeingRelit;
    }

    inline void setBlockLightBeingRelit(bool val) {
        m_blockLightBeingRelit = val;
    }

    void clearSkyLight();

    void clearBlockLight();

    void clearBlocksAndLight();

    void compressBlocks();

    void compressSkyLight();

    void compressBlockLight();

    void compressBlocksAndLight();

    void uncompressBlocksAndLight();

    inline void incrementPlayerCount() {
        m_playerCount++;
    }

    inline void decrementPlayerCount() {
        m_playerCount--;
    }

    inline bool hasNoPlayers() {
        return m_playerCount == 0;
    }

    inline uint16_t getPlayerCount() const {
        return m_playerCount;
    }

    inline uint16_t getLayerBlockType(uint32_t layerNum) {
        return m_layerBlockTypes[layerNum];
    }
};

}  // namespace lonelycube
