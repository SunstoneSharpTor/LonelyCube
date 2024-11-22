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

#include "core/lighting.h"

#include "core/pch.h"

#include "core/chunk.h"
#include "core/utils/iVec3.h"

void Lighting::propagateSkyLight(IVec3 pos, std::unordered_map<IVec3, Chunk>& worldChunks,
    bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, const ResourcePack& resourcePack,
    uint32_t modifiedBlock)
{
    Chunk& chunk = worldChunks.at(pos);
    int chunkPosition[3] = { pos.x, pos.y, pos.z };
    const std::array<Chunk*, 6> neighbouringChunks = { &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1] - 1, chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1], chunkPosition[2] - 1)),
                                                       &worldChunks.at(IVec3(chunkPosition[0] - 1, chunkPosition[1], chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0] + 1, chunkPosition[1], chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1], chunkPosition[2] + 1)),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1] + 1, chunkPosition[2])) };
    if (!chunk.isSkyLightBeingRelit()) {
        Chunk::s_checkingNeighbouringRelights.lock();
        bool neighbourBeingRelit = true;
        while (neighbourBeingRelit) {
            neighbourBeingRelit = false;
            for (uint32_t i = 0; i < 6; i++) {
                neighbourBeingRelit |= neighbouringChunks[i]->isSkyLightBeingRelit();
            }
            if (neighbourBeingRelit) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }
    Chunk::s_checkingNeighbouringRelights.unlock();

    std::queue<uint32_t> lightQueue;
    //add the the updated block to the light queue if it has been provided
    if (modifiedBlock < constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE) {
        if (modifiedBlock < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[5])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[5]);
        }
        if ((modifiedBlock % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[4])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[4]);
        }
        if ((modifiedBlock % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[3])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[3]);
        }
        if ((modifiedBlock % constants::CHUNK_SIZE) >= 1
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[2])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[2]);
        }
        if ((modifiedBlock % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[1])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[1]);
        }
        if (modifiedBlock >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[0])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[0]);
        }
    }
    //add the light values from chunk borders to the light queue
    uint32_t blockNum = 0;
    for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
        for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
            addSkyLightValuesFromBorder(chunk, neighbouringChunks[0], blockNum,
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                resourcePack, lightQueue);
            blockNum++;
        }
    }
    blockNum = 0;
    for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
        for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
            addSkyLightValuesFromBorder(chunk, neighbouringChunks[1], blockNum,
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                resourcePack, lightQueue);
            blockNum++;
        }
        blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    }
    blockNum = 0;
    for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
        for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
            addSkyLightValuesFromBorder(chunk, neighbouringChunks[2], blockNum,
                blockNum + constants::CHUNK_SIZE - 1, resourcePack, lightQueue);
            blockNum += constants::CHUNK_SIZE;
        }
    }
    blockNum = constants::CHUNK_SIZE - 1;
    for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
        for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
            addSkyLightValuesFromBorder(chunk, neighbouringChunks[3], blockNum,
                blockNum - constants::CHUNK_SIZE + 1, resourcePack, lightQueue);
            blockNum += constants::CHUNK_SIZE;
        }
    }
    blockNum = constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
        for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
            addSkyLightValuesFromBorder(chunk, neighbouringChunks[4], blockNum,
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                resourcePack, lightQueue);
            blockNum++;
        }
        blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    }
    blockNum = constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
        for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
            int8_t currentSkyLight = chunk.getSkyLight(blockNum);
            int8_t neighbouringSkyLight = neighbouringChunks[5]->getSkyLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
            //propogate direct skylight down without reducing its intensity
            neighbouringSkyLight += (neighbouringSkyLight == constants::skyLightMaxValue - 1)
                * !(resourcePack.getBlockData(neighbouringChunks[5]->getBlock(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).dimsLight);
            bool newHighestSkyLight = (currentSkyLight < neighbouringSkyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(blockNum, neighbouringSkyLight);
                lightQueue.push(blockNum);
            }
            blockNum++;
        }
    }
    //propogate the light values to the neighbouring blocks in the chunk
    for (int8_t i = 0; i < 6; i++) {
        neighbouringChunksToBeRelit[i] = false;
        chunksToRemesh[i] = false;
    }
    while (!lightQueue.empty()) {
        uint32_t blockNum = lightQueue.front();
        uint8_t skyLight = chunk.getSkyLight(blockNum);
        skyLight -= 1 * skyLight > 0;
        lightQueue.pop();
        if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            propagateSkyLightToBlock(chunk, blockNum, 5, skyLight, resourcePack, lightQueue);
        }
        else {
            propagateSkyLightToChunk(neighbouringChunks[5],
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                skyLight, resourcePack, neighbouringChunksToBeRelit[5]);
            chunksToRemesh[5] = true;
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            propagateSkyLightToBlock(chunk, blockNum, 4, skyLight, resourcePack, lightQueue);
        }
        else {
            propagateSkyLightToChunk(neighbouringChunks[4],
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                skyLight, resourcePack, neighbouringChunksToBeRelit[4]);
            chunksToRemesh[4] = true;
        }
        if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
            propagateSkyLightToBlock(chunk, blockNum, 3, skyLight, resourcePack, lightQueue);
        }
        else {
            propagateSkyLightToChunk(neighbouringChunks[3], blockNum - constants::CHUNK_SIZE + 1,
                skyLight, resourcePack, neighbouringChunksToBeRelit[3]);
            chunksToRemesh[3] = true;
        }
        if ((blockNum % constants::CHUNK_SIZE) >= 1) {
            propagateSkyLightToBlock(chunk, blockNum, 2, skyLight, resourcePack, lightQueue);
        }
        else {
            propagateSkyLightToChunk(neighbouringChunks[2], blockNum + constants::CHUNK_SIZE - 1,
                skyLight, resourcePack, neighbouringChunksToBeRelit[2]);
            chunksToRemesh[2] = true;
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
            propagateSkyLightToBlock(chunk, blockNum, 1, skyLight, resourcePack, lightQueue);
        }
        else {
            propagateSkyLightToChunk(neighbouringChunks[1],
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                skyLight, resourcePack, neighbouringChunksToBeRelit[1]);
            chunksToRemesh[1] = true;
        }
        if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[0];
            skyLight += (skyLight == constants::skyLightMaxValue - 1) * !(resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).dimsLight);
            uint8_t neighbouringSkyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringSkyLight < skyLight) * resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool dimsLight = resourcePack.getBlockData(neighbouringChunks[0]->getBlock(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).dimsLight;
            skyLight += (skyLight == constants::skyLightMaxValue - 1) * !(dimsLight);
            bool transparent = resourcePack.getBlockData(neighbouringChunks[0]->getBlock(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = neighbouringChunks[0]->getSkyLight(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[0] = true;
            }
            chunksToRemesh[0] = true;
        }
    }
}

void Lighting::propagateBlockLight(IVec3 pos, std::unordered_map<IVec3, Chunk>& worldChunks,
    bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, const ResourcePack& resourcePack,
    uint32_t modifiedBlock)
{
    Chunk& chunk = worldChunks.at(pos);
    int chunkPosition[3] = { pos.x, pos.y, pos.z };
    const std::array<Chunk*, 6> neighbouringChunks = { &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1] - 1, chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1], chunkPosition[2] - 1)),
                                                       &worldChunks.at(IVec3(chunkPosition[0] - 1, chunkPosition[1], chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0] + 1, chunkPosition[1], chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1], chunkPosition[2] + 1)),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1] + 1, chunkPosition[2])) };

    std::queue<uint32_t> lightQueue;
    //add the the updated block to the light queue if it has been provided
    if (modifiedBlock < constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE) {
        chunk.setBlockLight(modifiedBlock, resourcePack.getBlockData(chunk.getBlock(modifiedBlock)).blockLight);
        lightQueue.push(modifiedBlock);
        if (modifiedBlock < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[5])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[5]);
        }
        if ((modifiedBlock % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[4])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[4]);
        }
        if ((modifiedBlock % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[3])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[3]);
        }
        if ((modifiedBlock % constants::CHUNK_SIZE) >= 1
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[2])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[2]);
        }
        if ((modifiedBlock % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[1])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[1]);
        }
        if (modifiedBlock >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)
            && resourcePack.getBlockData(chunk.getBlock(modifiedBlock + chunk.neighbouringBlocks[0])).transparent) {
            lightQueue.push(modifiedBlock + chunk.neighbouringBlocks[0]);
        }
    }
    //add the light values from chunk borders to the light queue
    uint32_t blockNum = 0;
    for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
        for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
            addBlockLightValuesFromBorder(chunk, neighbouringChunks[0], blockNum,
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                resourcePack, lightQueue);
            blockNum++;
        }
    }
    blockNum = 0;
    for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
        for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
            addBlockLightValuesFromBorder(chunk, neighbouringChunks[1], blockNum,
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                resourcePack, lightQueue);
            blockNum++;
        }
        blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    }
    blockNum = 0;
    for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
        for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
            addBlockLightValuesFromBorder(chunk, neighbouringChunks[2], blockNum,
                blockNum + constants::CHUNK_SIZE - 1, resourcePack, lightQueue);
            blockNum += constants::CHUNK_SIZE;
        }
    }
    blockNum = constants::CHUNK_SIZE - 1;
    for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
        for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
            addBlockLightValuesFromBorder(chunk, neighbouringChunks[3], blockNum,
                blockNum - constants::CHUNK_SIZE + 1, resourcePack, lightQueue);
            blockNum += constants::CHUNK_SIZE;
        }
    }
    blockNum = constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
        for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
            addBlockLightValuesFromBorder(chunk, neighbouringChunks[4], blockNum,
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                resourcePack, lightQueue);
            blockNum++;
        }
        blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    }
    blockNum = constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
        for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
            addBlockLightValuesFromBorder(chunk, neighbouringChunks[5], blockNum,
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                resourcePack, lightQueue);
            blockNum++;
        }
    }
    //propogate the light values to the neighbouring blocks in the chunk
    for (int8_t i = 0; i < 6; i++) {
        neighbouringChunksToBeRelit[i] = false;
        chunksToRemesh[i] = false;
    }
    while (!lightQueue.empty()) {
        uint32_t blockNum = lightQueue.front();
        uint8_t blockLight = chunk.getBlockLight(blockNum);
        blockLight -= 1 * blockLight > 0;
        lightQueue.pop();
        if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            propagateBlockLightToBlock(chunk, blockNum, 5, blockLight, resourcePack, lightQueue);
        }
        else {
            propagateBlockLightToChunk(neighbouringChunks[5],
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                blockLight, resourcePack, neighbouringChunksToBeRelit[5]);
            chunksToRemesh[5] = true;
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            propagateBlockLightToBlock(chunk, blockNum, 4, blockLight, resourcePack, lightQueue);
        }
        else {
            propagateBlockLightToChunk(neighbouringChunks[4],
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                blockLight, resourcePack, neighbouringChunksToBeRelit[4]);
            chunksToRemesh[4] = true;
        }
        if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
            propagateBlockLightToBlock(chunk, blockNum, 3, blockLight, resourcePack, lightQueue);
        }
        else {
            propagateBlockLightToChunk(neighbouringChunks[3], blockNum - constants::CHUNK_SIZE + 1,
                blockLight, resourcePack, neighbouringChunksToBeRelit[3]);
            chunksToRemesh[3] = true;
        }
        if ((blockNum % constants::CHUNK_SIZE) >= 1) {
            propagateBlockLightToBlock(chunk, blockNum, 2, blockLight, resourcePack, lightQueue);
        }
        else {
            propagateBlockLightToChunk(neighbouringChunks[2], blockNum + constants::CHUNK_SIZE - 1,
                blockLight, resourcePack, neighbouringChunksToBeRelit[2]);
            chunksToRemesh[2] = true;
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
            propagateBlockLightToBlock(chunk, blockNum, 1, blockLight, resourcePack, lightQueue);
        }
        else {
            propagateBlockLightToChunk(neighbouringChunks[1],
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                blockLight, resourcePack, neighbouringChunksToBeRelit[1]);
            chunksToRemesh[1] = true;
        }
        if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
            propagateBlockLightToBlock(chunk, blockNum, 0, blockLight, resourcePack, lightQueue);
        }
        else {
            propagateBlockLightToChunk(neighbouringChunks[0],
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                blockLight, resourcePack, neighbouringChunksToBeRelit[0]);
            chunksToRemesh[0] = true;
        }
    }
}

void Lighting::propagateSkyDarkness(IVec3 pos, std::unordered_map<IVec3, Chunk>& worldChunks,
    bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, const ResourcePack& resourcePack,
    uint32_t modifiedBlock)
{
    Chunk& chunk = worldChunks.at(pos);
    int chunkPosition[3] = { pos.x, pos.y, pos.z };
    const std::array<Chunk*, 6> neighbouringChunks = { &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1] - 1, chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1], chunkPosition[2] - 1)),
                                                       &worldChunks.at(IVec3(chunkPosition[0] - 1, chunkPosition[1], chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0] + 1, chunkPosition[1], chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1], chunkPosition[2] + 1)),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1] + 1, chunkPosition[2])) };

    std::queue<uint32_t> lightQueue;
    // Add the the updated block to the light queue if it has been provided
    if (modifiedBlock < constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE) {
        lightQueue.push(modifiedBlock);
        chunk.setSkyLight(modifiedBlock, constants::skyLightMaxValue);
    }
    else {
        //add the light values from chunk borders to the light queue
        uint32_t blockNum = 0;
        for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
            for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
                addSkyDarknessValuesFromBorder(chunk, neighbouringChunks[0], blockNum, lightQueue,
                    blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
                blockNum++;
            }
        }
        blockNum = 0;
        for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
            for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
                addSkyDarknessValuesFromBorder(chunk, neighbouringChunks[1], blockNum, lightQueue,
                    blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
                blockNum++;
            }
            blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
        }
        blockNum = 0;
        for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
            for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
                addSkyDarknessValuesFromBorder(chunk, neighbouringChunks[2], blockNum, lightQueue,
                    blockNum + constants::CHUNK_SIZE - 1);
                blockNum += constants::CHUNK_SIZE;
            }
        }
        blockNum = constants::CHUNK_SIZE - 1;
        for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
            for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
                addSkyDarknessValuesFromBorder(chunk, neighbouringChunks[3], blockNum, lightQueue,
                    blockNum - constants::CHUNK_SIZE + 1);
                blockNum += constants::CHUNK_SIZE;
            }
        }
        blockNum = constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
        for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
            for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
                addSkyDarknessValuesFromBorder(chunk, neighbouringChunks[4], blockNum, lightQueue,
                    blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
                blockNum++;
            }
            blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
        }
        blockNum = constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
        for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
            for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
                addSkyDarknessValuesFromBorder(chunk, neighbouringChunks[5], blockNum, lightQueue,
                    blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
                blockNum++;
            }
        }
    }
    // Propogate the decreased light values to the neighbouring blocks in the chunk
    for (int8_t i = 0; i < 6; i++) {
        neighbouringChunksToBeRelit[i] = false;
        chunksToRemesh[i] = false;
    }
    while (!lightQueue.empty()) {
        // Find the new value of the block
        uint32_t blockNum = lightQueue.front();
        uint8_t skyLight = chunk.getSkyLight(blockNum);
        lightQueue.pop();
        uint8_t neighbouringSkyLights[6];
        if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[5];
            neighbouringSkyLights[5] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[5] = neighbouringChunks[5]->getSkyLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            chunksToRemesh[5] = true;
        }
        uint8_t highestNeighbourSkyLight = neighbouringSkyLights[5];
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[4];
            neighbouringSkyLights[4] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[4] = neighbouringChunks[4]->getSkyLight(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            chunksToRemesh[4] = true;
        }
        highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[4]);
        if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[3];
            neighbouringSkyLights[3] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[3] = neighbouringChunks[3]->getSkyLight(
                blockNum - (constants::CHUNK_SIZE - 1));
            chunksToRemesh[3] = true;
        }
        highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[3]);
        if ((blockNum % constants::CHUNK_SIZE) >= 1) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[2];
            neighbouringSkyLights[2] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[2] = neighbouringChunks[2]->getSkyLight(
                blockNum + (constants::CHUNK_SIZE - 1));
            chunksToRemesh[2] = true;
        }
        highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[2]);
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[1];
            neighbouringSkyLights[1] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[1] = neighbouringChunks[1]->getSkyLight(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            chunksToRemesh[1] = true;
        }
        highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[1]);
        if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[0];
            neighbouringSkyLights[0] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[0] = neighbouringChunks[0]->getSkyLight(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            chunksToRemesh[0] = true;
        }
        highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[0]);
        bool skyAccess = neighbouringSkyLights[5] == constants::skyLightMaxValue && !resourcePack.getBlockData(chunk.getBlock(blockNum)).dimsLight;
        highestNeighbourSkyLight -= !skyAccess && highestNeighbourSkyLight > 0;
        highestNeighbourSkyLight *= resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
        if (highestNeighbourSkyLight < skyLight) {
            chunk.setSkyLight(blockNum, highestNeighbourSkyLight);
            // Add the neighbouring blocks to the queue if they have a low enough sky light
            highestNeighbourSkyLight += highestNeighbourSkyLight == 0;
            if (neighbouringSkyLights[5] < skyLight && neighbouringSkyLights[5] >= highestNeighbourSkyLight) {
                if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
                    uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[5];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[5] =  true;
                }
            }
            if (neighbouringSkyLights[4] < skyLight && neighbouringSkyLights[4] >= highestNeighbourSkyLight) {
                if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
                    uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[4];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[4] =  true;
                }
            }
            if (neighbouringSkyLights[3] < skyLight && neighbouringSkyLights[3] >= highestNeighbourSkyLight) {
                if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
                    uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[3];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[3] =  true;
                }
            }
            if (neighbouringSkyLights[2] < skyLight && neighbouringSkyLights[2] >= highestNeighbourSkyLight) {
                if ((blockNum % constants::CHUNK_SIZE) >= 1) {
                    uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[2];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[2] =  true;
                }
            }
            if (neighbouringSkyLights[1] < skyLight && neighbouringSkyLights[1] >= highestNeighbourSkyLight) {
                if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
                    uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[1];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[1] =  true;
                }
            }
            if ((neighbouringSkyLights[0] < skyLight || skyLight == constants::skyLightMaxValue) && neighbouringSkyLights[0] >= highestNeighbourSkyLight) {
                if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
                    uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[0];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[0] =  true;
                }
            }
        }
    }
}

void Lighting::propagateBlockDarkness(IVec3 pos, std::unordered_map<IVec3, Chunk>& worldChunks,
    bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, const ResourcePack& resourcePack,
    uint32_t modifiedBlock)
{
    Chunk& chunk = worldChunks.at(pos);
    int chunkPosition[3] = { pos.x, pos.y, pos.z };
    const std::array<Chunk*, 6> neighbouringChunks = { &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1] - 1, chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1], chunkPosition[2] - 1)),
                                                       &worldChunks.at(IVec3(chunkPosition[0] - 1, chunkPosition[1], chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0] + 1, chunkPosition[1], chunkPosition[2])),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1], chunkPosition[2] + 1)),
                                                       &worldChunks.at(IVec3(chunkPosition[0], chunkPosition[1] + 1, chunkPosition[2])) };

    std::queue<uint32_t> lightQueue;
    // Add the the updated block to the light queue if it has been provided
    if (modifiedBlock < constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE) {
        lightQueue.push(modifiedBlock);
        chunk.setBlockLight(modifiedBlock, constants::blockLightMaxValue);
    }
    else {
        //add the light values from chunk borders to the light queue
        uint32_t blockNum = 0;
        for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
            for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
                addBlockDarknessValuesFromBorder(chunk, neighbouringChunks[0], blockNum,
                    blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                    resourcePack, lightQueue);
                blockNum++;
            }
        }
        blockNum = 0;
        for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
            for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
                addBlockDarknessValuesFromBorder(chunk, neighbouringChunks[1], blockNum,
                    blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                    resourcePack, lightQueue);
                blockNum++;
            }
            blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
        }
        blockNum = 0;
        for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
            for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
                addBlockDarknessValuesFromBorder(chunk, neighbouringChunks[2], blockNum,
                    blockNum + constants::CHUNK_SIZE - 1, resourcePack, lightQueue);
                blockNum += constants::CHUNK_SIZE;
            }
        }
        blockNum = constants::CHUNK_SIZE - 1;
        for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
            for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
                addBlockDarknessValuesFromBorder(chunk, neighbouringChunks[3], blockNum,
                    blockNum - constants::CHUNK_SIZE + 1, resourcePack, lightQueue);
                blockNum += constants::CHUNK_SIZE;
            }
        }
        blockNum = constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
        for (uint32_t y = 0; y < constants::CHUNK_SIZE; y++) {
            for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
                addBlockDarknessValuesFromBorder(chunk, neighbouringChunks[4], blockNum,
                    blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                    resourcePack, lightQueue);
                blockNum++;
            }
            blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
        }
        blockNum = constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
        for (uint32_t z = 0; z < constants::CHUNK_SIZE; z++) {
            for (uint32_t x = 0; x < constants::CHUNK_SIZE; x++) {
                addBlockDarknessValuesFromBorder(chunk, neighbouringChunks[5], blockNum,
                    blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1),
                    resourcePack, lightQueue);
                blockNum++;
            }
        }
    }
    // Propogate the decreased light values to the neighbouring blocks in the chunk
    for (int8_t i = 0; i < 6; i++) {
        neighbouringChunksToBeRelit[i] = false;
        chunksToRemesh[i] = false;
    }
    while (!lightQueue.empty()) {
        // Find the new value of the block
        uint32_t blockNum = lightQueue.front();
        uint8_t blockLight = chunk.getBlockLight(blockNum);
        lightQueue.pop();
        uint8_t neighbouringBlockLights[6];
        if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[5];
            neighbouringBlockLights[5] = chunk.getBlockLight(neighbouringBlockNum);
        }
        else {
            neighbouringBlockLights[5] = neighbouringChunks[5]->getBlockLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            chunksToRemesh[5] = true;
        }
        uint8_t highestNeighbourBlockLight = neighbouringBlockLights[5];
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[4];
            neighbouringBlockLights[4] = chunk.getBlockLight(neighbouringBlockNum);
        }
        else {
            neighbouringBlockLights[4] = neighbouringChunks[4]->getBlockLight(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            chunksToRemesh[4] = true;
        }
        highestNeighbourBlockLight = std::max(highestNeighbourBlockLight, neighbouringBlockLights[4]);
        if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[3];
            neighbouringBlockLights[3] = chunk.getBlockLight(neighbouringBlockNum);
        }
        else {
            neighbouringBlockLights[3] = neighbouringChunks[3]->getBlockLight(
                blockNum - (constants::CHUNK_SIZE - 1));
            chunksToRemesh[3] = true;
        }
        highestNeighbourBlockLight = std::max(highestNeighbourBlockLight, neighbouringBlockLights[3]);
        if ((blockNum % constants::CHUNK_SIZE) >= 1) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[2];
            neighbouringBlockLights[2] = chunk.getBlockLight(neighbouringBlockNum);
        }
        else {
            neighbouringBlockLights[2] = neighbouringChunks[2]->getBlockLight(
                blockNum + (constants::CHUNK_SIZE - 1));
            chunksToRemesh[2] = true;
        }
        highestNeighbourBlockLight = std::max(highestNeighbourBlockLight, neighbouringBlockLights[2]);
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[1];
            neighbouringBlockLights[1] = chunk.getBlockLight(neighbouringBlockNum);
        }
        else {
            neighbouringBlockLights[1] = neighbouringChunks[1]->getBlockLight(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            chunksToRemesh[1] = true;
        }
        highestNeighbourBlockLight = std::max(highestNeighbourBlockLight, neighbouringBlockLights[1]);
        if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
            uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[0];
            neighbouringBlockLights[0] = chunk.getBlockLight(neighbouringBlockNum);
        }
        else {
            neighbouringBlockLights[0] = neighbouringChunks[0]->getBlockLight(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            chunksToRemesh[0] = true;
        }
        highestNeighbourBlockLight = std::max(highestNeighbourBlockLight, neighbouringBlockLights[0]);
        highestNeighbourBlockLight -= highestNeighbourBlockLight > 0;
        highestNeighbourBlockLight *= resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
        if (highestNeighbourBlockLight < blockLight) {
            chunk.setBlockLight(blockNum, highestNeighbourBlockLight);
            // Add the neighbouring blocks to the queue if they have a low enough block light
            highestNeighbourBlockLight += highestNeighbourBlockLight == 0;
            if (neighbouringBlockLights[5] < blockLight && neighbouringBlockLights[5] >= highestNeighbourBlockLight) {
                uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[5];
                if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))
                    && neighbouringBlockLights[5] > resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).blockLight)
                {
                    lightQueue.push(neighbouringBlockNum);
                }
                else
                {
                    neighbouringChunksToBeRelit[5] =  true;
                }
            }
            if (neighbouringBlockLights[4] < blockLight && neighbouringBlockLights[4] >= highestNeighbourBlockLight) {
                uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[4];
                if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))
                    && neighbouringBlockLights[4] > resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).blockLight)
                {
                    lightQueue.push(neighbouringBlockNum);
                }
                else
                {
                    neighbouringChunksToBeRelit[4] =  true;
                }
            }
            if (neighbouringBlockLights[3] < blockLight && neighbouringBlockLights[3] >= highestNeighbourBlockLight) {
                uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[3];
                if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)
                    && neighbouringBlockLights[3] > resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).blockLight)
                {
                    lightQueue.push(neighbouringBlockNum);
                }
                else
                {
                    neighbouringChunksToBeRelit[3] =  true;
                }
            }
            if (neighbouringBlockLights[2] < blockLight && neighbouringBlockLights[2] >= highestNeighbourBlockLight) {
                uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[2];
                if ((blockNum % constants::CHUNK_SIZE) >= 1
                    && neighbouringBlockLights[2] > resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).blockLight)
                {
                    lightQueue.push(neighbouringBlockNum);
                }
                else
                {
                    neighbouringChunksToBeRelit[2] =  true;
                }
            }
            if (neighbouringBlockLights[1] < blockLight && neighbouringBlockLights[1] >= highestNeighbourBlockLight) {
                uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[1];
                if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE
                    && neighbouringBlockLights[1] > resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).blockLight)
                {
                    lightQueue.push(neighbouringBlockNum);
                }
                else
                {
                    neighbouringChunksToBeRelit[1] =  true;
                }
            }
            if (neighbouringBlockLights[0] < blockLight && neighbouringBlockLights[0] >= highestNeighbourBlockLight) {
                uint32_t neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[0];
                if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)
                    && neighbouringBlockLights[0] > resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).blockLight)
                {
                    lightQueue.push(neighbouringBlockNum);
                }
                else
                {
                    neighbouringChunksToBeRelit[0] =  true;
                }
            }
        }
    }
}

void Lighting::relightChunksAroundBlock(const IVec3& blockCoords, const IVec3& chunkPosition,
    uint8_t originalBlock, uint8_t newBlock, std::vector<IVec3>& chunksToRemesh,
    std::unordered_map<IVec3, Chunk>& worldChunks, const ResourcePack& resourcePack)
{
    uint32_t modifiedBlockNum = blockCoords.x - chunkPosition.x * constants::CHUNK_SIZE
        + (blockCoords.y - chunkPosition.y * constants::CHUNK_SIZE) * constants::CHUNK_SIZE
        * constants::CHUNK_SIZE + (blockCoords.z - chunkPosition.z * constants::CHUNK_SIZE)
        * constants::CHUNK_SIZE;
    BlockData oldBlockData = resourcePack.getBlockData(originalBlock);
    BlockData newBlockData = resourcePack.getBlockData(newBlock);

    // j = 0 represents sky lighting and j = 1 represents block lighting
    for (int j = 0; j < 2; j++) {
        bool darken, lighten;
        if (j == 0)
        {
            // Determine whether light or darkness needs to be propagated for sky light (or both)
            darken = (oldBlockData.transparent && !newBlockData.transparent) ||
                ((!oldBlockData.dimsLight && oldBlockData.transparent) && newBlockData.dimsLight);
            lighten = (!oldBlockData.transparent && newBlockData.transparent) ||
                (oldBlockData.dimsLight && !newBlockData.dimsLight);
        }
        else
        {
            // Determine whether light or darkness needs to be propagated for block light (or both)
            darken = (oldBlockData.transparent && !newBlockData.transparent)
                || newBlockData.blockLight < oldBlockData.blockLight;
            lighten = (!oldBlockData.transparent && newBlockData.transparent)
                || newBlockData.blockLight > oldBlockData.blockLight;
        }

        std::vector<IVec3> relitChunks;
        int numChunksLightened = 0;
        int numChunksDarkened = 0;
        int numSpreads = 0;
        // i = 0 represents propagating light and i = 1 represents propagating darkness
        for (int i = 0; i < 2; i++) {
            if ((i == 0 && !lighten) || (i == 1 && !darken)) {
                continue;
            }
            // Add the chunk containing the modified block to the vector
            std::vector<IVec3> chunksToBeRelit;
            chunksToBeRelit.emplace_back(chunkPosition);
            while (chunksToBeRelit.size() > 0) {
                if (std::find(relitChunks.begin(), relitChunks.end(), chunksToBeRelit[0]) == relitChunks.end()) {
                    relitChunks.push_back(chunksToBeRelit[0]);
                }
                bool neighbouringChunksToRelight[6];
                bool neighbouringChunksToRemesh[6];
                IVec3 neighbouringChunkPositions[6];
                bool neighbouringChunksLoaded = true;
                // Check that the chunk has its neighbours loaded so that it can be lit
                for (int8_t ii = 0; ii < 6; ii++) {
                    neighbouringChunkPositions[ii] = chunksToBeRelit[0] + s_neighbouringChunkOffsets[ii];
                    if (!worldChunks.contains(neighbouringChunkPositions[ii])) {
                        neighbouringChunksLoaded = false;
                        break;
                    }
                }
                // If the chunk's neighbours aren't loaded, remove the chunk as it cannot be lit correctly
                if (!neighbouringChunksLoaded) {
                    worldChunks.at(chunksToBeRelit[0]).setSkyLightToBeOutdated();
                    std::vector<IVec3>::iterator it = chunksToBeRelit.begin();
                    chunksToBeRelit.erase(it);
                    continue;
                }

                // Calculate the lighting on the current chunk
                if (i == 0) {
                    if (numChunksLightened == 0) {
                        if (j == 0)
                            propagateSkyLight(chunksToBeRelit[0], worldChunks,
                                neighbouringChunksToRelight, neighbouringChunksToRemesh,
                                resourcePack, modifiedBlockNum);
                        else
                            propagateBlockLight(chunksToBeRelit[0], worldChunks,
                                neighbouringChunksToRelight, neighbouringChunksToRemesh,
                                resourcePack, modifiedBlockNum);
                    }
                    else {
                        if (j == 0)
                            propagateSkyLight(chunksToBeRelit[0], worldChunks,
                                neighbouringChunksToRelight, neighbouringChunksToRemesh,
                                resourcePack);
                        else
                            propagateBlockLight(chunksToBeRelit[0], worldChunks,
                                neighbouringChunksToRelight, neighbouringChunksToRemesh,
                                resourcePack);
                    }
                    numChunksLightened++;
                }
                else {
                    if (numChunksDarkened == 0) {
                        if (j == 0)
                            propagateSkyDarkness(chunksToBeRelit[0], worldChunks,
                                neighbouringChunksToRelight, neighbouringChunksToRemesh,
                                resourcePack, modifiedBlockNum);
                        else
                            propagateBlockDarkness(chunksToBeRelit[0], worldChunks,
                                neighbouringChunksToRelight, neighbouringChunksToRemesh,
                                resourcePack, modifiedBlockNum);
                    }
                    else {
                        if (j == 0)
                            propagateSkyDarkness(chunksToBeRelit[0], worldChunks,
                                neighbouringChunksToRelight, neighbouringChunksToRemesh,
                                resourcePack);
                        else
                            propagateBlockDarkness(chunksToBeRelit[0], worldChunks,
                                neighbouringChunksToRelight, neighbouringChunksToRemesh,
                                resourcePack);
                    }
                    numChunksDarkened++;
                }

                // Add the current chunk to the chunksToBeRemeshed
                if (std::find(chunksToRemesh.begin(), chunksToRemesh.end(), chunksToBeRelit[0]) == chunksToRemesh.end()) {
                    chunksToRemesh.push_back(chunksToBeRelit[0]);
                }
                // Add relevant neighbouring chunks to the chunksToBeRemeshed
                for (int8_t i = 0; i < 6; i++) {
                    if (neighbouringChunksToRemesh[i]) {
                        IVec3 chunkToBeRemeshed = chunksToBeRelit[0];
                        chunkToBeRemeshed += s_neighbouringChunkOffsets[i];
                        if (std::find(chunksToRemesh.begin(), chunksToRemesh.end(), chunkToBeRemeshed) == chunksToRemesh.end()) {
                            chunksToRemesh.push_back(chunkToBeRemeshed);
                        }
                    }
                }
                std::vector<IVec3>::iterator it = chunksToBeRelit.begin();
                chunksToBeRelit.erase(it);
                //add the neighbouring chunks that were marked as needing recalculating to the queue
                for (int8_t i = 0; i < 6; i++) {
                    if (!neighbouringChunksToRelight[i]) {
                        continue;
                    }
                    if (std::find(chunksToBeRelit.begin(), chunksToBeRelit.end(), neighbouringChunkPositions[i]) == chunksToBeRelit.end()) {
                        chunksToBeRelit.push_back(neighbouringChunkPositions[i]);
                        numSpreads++;
                    }
                }
            }
        }
        for (IVec3 position : relitChunks) {
            worldChunks.at(position).compressSkyLight();
        }
        // std::cout << numChunksLightened + numChunksDarkened << " chunks relit\n";
    }
}
