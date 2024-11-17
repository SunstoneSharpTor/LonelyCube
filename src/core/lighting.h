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

#include "core/chunk.h"
#include "core/position.h"
#include "core/resourcePack.h"

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
        bool newHighestSkyLight = (currentSkyLight < neighbouringSkyLight) *
            resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
        if (newHighestSkyLight) {
            chunk.setSkyLight(blockNum, neighbouringSkyLight);
            lightQueue.push(blockNum);
        }
    }

    inline static void addBlockLightValuesFromBorder(Chunk& chunk, Chunk* neighbouringChunk, const
        uint32_t blockNum, const uint32_t neighbouringBlockNum, const ResourcePack& resourcePack,
        std::queue<uint32_t>& lightQueue)
    {
        int8_t currentBlockLight = chunk.getBlockLight(blockNum);
        int8_t neighbouringBlockLight = neighbouringChunk->getBlockLight(neighbouringBlockNum) - 1;
        bool newHighestBlockLight = (currentBlockLight < neighbouringBlockLight) *
            resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
        if (newHighestBlockLight) {
            chunk.setBlockLight(blockNum, neighbouringBlockLight);
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
        bool newHighestSkyLight = (neighbouringSkyLight < skyLight) *
            resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).transparent;
        if (newHighestSkyLight) {
            chunk.setSkyLight(neighbouringBlockNum, skyLight);
            lightQueue.push(neighbouringBlockNum);
        }
    }

    inline static void propagateBlockLightToBlock(Chunk& chunk, const uint32_t blockNum, uint8_t
        direction, uint8_t blockLight, const ResourcePack& resourcePack, std::queue<uint32_t>&
        lightQueue)
    {
        uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[direction];
        uint8_t neighbouringBlockLight = chunk.getBlockLight(neighbouringBlockNum);
        bool newHighestBlockLight = (neighbouringBlockLight < blockLight) *
            resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).transparent;
        if (newHighestBlockLight) {
            chunk.setBlockLight(neighbouringBlockNum, blockLight);
            lightQueue.push(neighbouringBlockNum);
        }
    }

    inline static void propagateSkyLightToChunk(Chunk* neighbouringChunk, const uint32_t
        neighbouringBlockNum, const uint8_t skyLight, const ResourcePack& resourcePack, bool&
        shouldRelightChunk)
    {
        bool transparent = resourcePack.getBlockData(neighbouringChunk->getBlock(
            neighbouringBlockNum)).transparent;
        bool newHighestSkyLight = neighbouringChunk->getSkyLight(neighbouringBlockNum) < skyLight;
        if (transparent && newHighestSkyLight) {
            shouldRelightChunk = true;
        }
    }

    inline static void propagateBlockLightToChunk(Chunk* neighbouringChunk, const uint32_t
        neighbouringBlockNum, const uint8_t blockLight, const ResourcePack& resourcePack, bool&
        shouldRelightChunk)
    {
        bool transparent = resourcePack.getBlockData(neighbouringChunk->getBlock(
            neighbouringBlockNum)).transparent;
        bool newHighestBlockLight = neighbouringChunk->getBlockLight(neighbouringBlockNum) < blockLight;
        if (transparent && newHighestBlockLight) {
            shouldRelightChunk = true;
        }
    }
};
