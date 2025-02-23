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

#include "core/chunk.h"
#include "core/utils/iVec3.h"
#include "core/resourcePack.h"

namespace lonelycube {

class Lighting {
public:
    // Propagates the sky light through the chunk
    static void propagateSkyLight(IVec3 chunkPosition, std::unordered_map<IVec3, Chunk>&
        worldChunks, bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, const ResourcePack&
        resourcePack, uint32_t modifiedBlock = constants::CHUNK_SIZE * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE);

    // Propagates the block light through the chunk
    static void propagateBlockLight(IVec3 chunkPosition, std::unordered_map<IVec3, Chunk>&
        worldChunks, bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, const ResourcePack&
        resourcePack, uint32_t modifiedBlock = constants::CHUNK_SIZE * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE);

    // Propagates the absence of sky light through the chunk
    static void propagateSkyDarkness(IVec3 chunkPosition, std::unordered_map<IVec3, Chunk>&
        worldChunks, bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, const ResourcePack&
        resourcePack, uint32_t modifiedBlock = constants::CHUNK_SIZE * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE);

    // Propagates the absence of block light through the chunk
    static void propagateBlockDarkness(IVec3 chunkPosition, std::unordered_map<IVec3, Chunk>&
        worldChunks, bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, const ResourcePack&
        resourcePack, uint32_t modifiedBlock = constants::CHUNK_SIZE * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE);

    // Recalculate all necessary lighting information for a block change
    static void relightChunksAroundBlock(const IVec3& blockCoords, const IVec3& chunkPosition,
        uint8_t originalBlock, uint8_t newBlock, std::vector<IVec3>& chunksToRemesh,
        std::unordered_map<IVec3, Chunk>& worldChunks, const ResourcePack& ResourcePack);
private:
    static const inline std::array<IVec3, 6> s_neighbouringChunkOffsets = { IVec3(0, -1, 0),
        IVec3(0, 0, -1),
        IVec3(-1, 0, 0),
        IVec3(1, 0, 0),
        IVec3(0, 0, 1),
        IVec3(0, 1, 0) };

    inline static void addSkyLightValuesFromBorder(Chunk& chunk, Chunk* neighbouringChunk, const
        uint32_t blockNum, const uint32_t neighbouringBlockNum, const ResourcePack& resourcePack,
        std::queue<uint32_t>& lightQueue)
    {
        int8_t currentSkyLight = chunk.getSkyLight(blockNum);
        int8_t neighbouringSkyLight = neighbouringChunk->getSkyLight(neighbouringBlockNum) - 1;
        if (currentSkyLight < neighbouringSkyLight) {
            chunk.setSkyLight(blockNum, neighbouringSkyLight);
            if (resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent)
                lightQueue.push(blockNum);
        }
    }

    inline static void addBlockLightValuesFromBorder(Chunk& chunk, Chunk* neighbouringChunk, const
        uint32_t blockNum, const uint32_t neighbouringBlockNum, const ResourcePack& resourcePack,
        std::queue<uint32_t>& lightQueue)
    {
        int8_t currentBlockLight = chunk.getBlockLight(blockNum);
        int8_t neighbouringBlockLight = neighbouringChunk->getBlockLight(neighbouringBlockNum) - 1;
        if (currentBlockLight < neighbouringBlockLight) {
            chunk.setBlockLight(blockNum, neighbouringBlockLight);
            if (resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent)
                lightQueue.push(blockNum);
        }
    }

    inline static void addSkyDarknessValuesFromBorder(Chunk& chunk, Chunk* neighbouringChunk, const
        uint32_t blockNum, std::queue<uint32_t>& lightQueue, const uint32_t neighbouringBlockNum)
    {
        int8_t currentSkyLight = chunk.getSkyLight(blockNum);
        int8_t neighbouringSkyLight = neighbouringChunk->getSkyLight(neighbouringBlockNum);
        if (currentSkyLight >= neighbouringSkyLight) {
            lightQueue.push(blockNum);
        }
    }

    inline static void addBlockDarknessValuesFromBorder(Chunk& chunk, Chunk* neighbouringChunk,
        const uint32_t blockNum, const uint32_t neighbouringBlockNum, const ResourcePack&
        resourcePack, std::queue<uint32_t>& lightQueue)
    {
        int8_t currentBlockLight = chunk.getBlockLight(blockNum);
        if (currentBlockLight > resourcePack.getBlockData(chunk.getBlock(blockNum)).blockLight) {
            int8_t neighbouringBlockLight = neighbouringChunk->getBlockLight(neighbouringBlockNum);
            if (currentBlockLight >= neighbouringBlockLight) {
                lightQueue.push(blockNum);
            }
        }
    }

    inline static void propagateSkyLightToBlock(Chunk& chunk, const uint32_t blockNum, uint8_t
        direction, uint8_t skyLight, const ResourcePack& resourcePack, std::queue<uint32_t>&
        lightQueue)
    {
        uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[direction];
        uint8_t neighbouringSkyLight = chunk.getSkyLight(neighbouringBlockNum);
        if (neighbouringSkyLight < skyLight) {
            chunk.setSkyLight(neighbouringBlockNum, skyLight);
            if (resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).transparent)
                lightQueue.push(neighbouringBlockNum);
        }
    }

    inline static void propagateBlockLightToBlock(Chunk& chunk, const uint32_t blockNum, uint8_t
        direction, uint8_t blockLight, const ResourcePack& resourcePack, std::queue<uint32_t>&
        lightQueue)
    {
        uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[direction];
        uint8_t neighbouringBlockLight = chunk.getBlockLight(neighbouringBlockNum);
        if (neighbouringBlockLight < blockLight) {
            chunk.setBlockLight(neighbouringBlockNum, blockLight);
            if (resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).transparent)
                lightQueue.push(neighbouringBlockNum);
        }
    }

    inline static void propagateSkyLightToChunk(Chunk* neighbouringChunk, const uint32_t
        neighbouringBlockNum, const uint8_t skyLight, const ResourcePack& resourcePack, bool&
        shouldRelightChunk)
    {
        if (neighbouringChunk->getSkyLight(neighbouringBlockNum) < skyLight) {
            shouldRelightChunk = true;
        }
    }

    inline static void propagateBlockLightToChunk(Chunk* neighbouringChunk, const uint32_t
        neighbouringBlockNum, const uint8_t blockLight, const ResourcePack& resourcePack, bool&
        shouldRelightChunk)
    {
        bool newHighestBlockLight = neighbouringChunk->getBlockLight(neighbouringBlockNum) < blockLight;
        if (newHighestBlockLight) {
            shouldRelightChunk = true;
        }
    }
};

}  // namespace lonelycube
