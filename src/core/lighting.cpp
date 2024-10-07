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
#include "core/position.h"

void Lighting::propagateSkyLight(Position pos, std::unordered_map<Position, Chunk>& worldChunks,
    bool* neighbouringChunksToBeRelit, ResourcePack& resourcePack, Position blockPosition) {
    Chunk& chunk = worldChunks.at(pos);
    int chunkPosition[3] = { pos.x, pos.y, pos.z };
    Position neighbouringChunkPositions[6] = { Position(chunkPosition[0], chunkPosition[1] - 1, chunkPosition[2]),
                                               Position(chunkPosition[0], chunkPosition[1], chunkPosition[2] - 1),
                                               Position(chunkPosition[0] - 1, chunkPosition[1], chunkPosition[2]),
                                               Position(chunkPosition[0] + 1, chunkPosition[1], chunkPosition[2]),
                                               Position(chunkPosition[0], chunkPosition[1], chunkPosition[2] + 1),
                                               Position(chunkPosition[0], chunkPosition[1] + 1, chunkPosition[2]) };
    if (!chunk.isSkyBeingRelit()) {
        Chunk::s_checkingNeighbouringRelights.lock();
        bool neighbourBeingRelit = true;
        while (neighbourBeingRelit) {
            neighbourBeingRelit = false;
            for (unsigned int i = 0; i < 6; i++) {
                neighbourBeingRelit |= worldChunks.at(neighbouringChunkPositions[i]).isSkyBeingRelit();
            }
            if (neighbourBeingRelit) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }
    Chunk::s_checkingNeighbouringRelights.unlock();

    std::queue<unsigned int> lightQueue;
    //add the the updated block to the light queue if it has been provided
    if (blockPosition != Position(constants::WORLD_BORDER_DISTANCE * 2,
        constants::WORLD_BORDER_DISTANCE * 2, constants::WORLD_BORDER_DISTANCE * 2)) {
        unsigned int blockNum = (blockPosition.x - pos.x * constants::CHUNK_SIZE) + (blockPosition.y
            - pos.y * constants::CHUNK_SIZE) * constants::CHUNK_SIZE * constants::CHUNK_SIZE +
            (blockPosition.z - pos.z * constants::CHUNK_SIZE) * constants::CHUNK_SIZE;
        lightQueue.push(blockNum);
    }
    //add the light values from chunk borders to the light queue
    unsigned int blockNum = 0;
    for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
        for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[0]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(blockNum, neighbouringskyLight);
                lightQueue.push(blockNum);
            }
            blockNum++;
        }
    }
    blockNum = 0;
    for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
        for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[1]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(blockNum, neighbouringskyLight);
                lightQueue.push(blockNum);
            }
            blockNum++;
        }
        blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    }
    blockNum = 0;
    for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
        for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[2]).getSkyLight(
                blockNum + constants::CHUNK_SIZE - 1) - 1;
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(blockNum, neighbouringskyLight);
                lightQueue.push(blockNum);
            }
            blockNum += constants::CHUNK_SIZE;
        }
    }
    blockNum = constants::CHUNK_SIZE - 1;
    for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
        for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[3]).getSkyLight(
                blockNum - constants::CHUNK_SIZE + 1) - 1;
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(blockNum, neighbouringskyLight);
                lightQueue.push(blockNum);
            }
            blockNum += constants::CHUNK_SIZE;
        }
    }
    blockNum = constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
        for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[4]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(blockNum, neighbouringskyLight);
                lightQueue.push(blockNum);
            }
            blockNum++;
        }
        blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    }
    blockNum = constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
        for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[5]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
            //propogate direct skylight down without reducing its intensity
            neighbouringskyLight += (neighbouringskyLight == 14)
                * !(resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[5]).getBlock(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).dimsLight);
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(blockNum, neighbouringskyLight);
                lightQueue.push(blockNum);
            }
            blockNum++;
        }
    }
    //propogate the light values to the neighbouring blocks in the chunk
    for (char i = 0; i < 6; i++) {
        neighbouringChunksToBeRelit[i] = false;
    }
    while (!lightQueue.empty()) {
        unsigned int blockNum = lightQueue.front();
        unsigned char skyLight = chunk.getSkyLight(blockNum) - 1;
        lightQueue.pop();
        if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[5];
            unsigned char neighbouringskyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool transparrent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[5]).getBlock(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[5]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparrent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[5] = true;
            }
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[4];
            unsigned char neighbouringskyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool transparrent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[4]).getBlock(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[4]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparrent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[4] = true;
            }
        }
        if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[3];
            unsigned char neighbouringskyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool transparrent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[3]).getBlock(
                blockNum - (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[3]).getSkyLight(
                blockNum - (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparrent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[3] = true;
            }
        }
        if ((blockNum % constants::CHUNK_SIZE) >= 1) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[2];
            unsigned char neighbouringskyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool transparrent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[2]).getBlock(
                blockNum + (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[2]).getSkyLight(
                blockNum + (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparrent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[2] = true;
            }
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[1];
            unsigned char neighbouringskyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool transparrent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[1]).getBlock(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[1]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparrent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[1] = true;
            }
        }
        if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[0];
            skyLight += (skyLight == 14) * !(resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).dimsLight);
            unsigned char neighbouringskyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunk.getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool dimsLight = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[0]).getBlock(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).dimsLight;
            skyLight += (skyLight == 14) * !(dimsLight);
            bool transparrent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[0]).getBlock(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[0]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparrent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[0] = true;
            }
        }
    }
    
    //(*m_worldInfo.numRelights)--;
    chunk.setSkyLightToBeUpToDate();
    //m_calculatingSkylight = false;
    // lock release
    // std::lock_guard<std::mutex> lock(chunk.accessingSkylightMtx);
    // m_accessingSkylightCV.notify_all();
}

void Lighting::propagateSkyDarkness(Position pos, std::unordered_map<Position, Chunk>& worldChunks,
    bool* neighbouringChunksToBeRelit, ResourcePack& resourcePack, Position blockPosition) {
    Chunk& chunk = worldChunks.at(pos);
    int chunkPosition[3] = { pos.x, pos.y, pos.z };
    Position neighbouringChunkPositions[6] = { Position(chunkPosition[0], chunkPosition[1] - 1, chunkPosition[2]),
                                               Position(chunkPosition[0], chunkPosition[1], chunkPosition[2] - 1),
                                               Position(chunkPosition[0] - 1, chunkPosition[1], chunkPosition[2]),
                                               Position(chunkPosition[0] + 1, chunkPosition[1], chunkPosition[2]),
                                               Position(chunkPosition[0], chunkPosition[1], chunkPosition[2] + 1),
                                               Position(chunkPosition[0], chunkPosition[1] + 1, chunkPosition[2]) };
    if (!chunk.isSkyBeingRelit()) {
        Chunk::s_checkingNeighbouringRelights.lock();
        bool neighbourBeingRelit = true;
        while (neighbourBeingRelit) {
            neighbourBeingRelit = false;
            for (unsigned int i = 0; i < 6; i++) {
                neighbourBeingRelit |= worldChunks.at(neighbouringChunkPositions[i]).isSkyBeingRelit();
            }
            if (neighbourBeingRelit) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }
    Chunk::s_checkingNeighbouringRelights.unlock();

    std::queue<unsigned int> lightQueue;
    //add the the updated block to the light queue if it has been provided
    if (blockPosition != Position(constants::WORLD_BORDER_DISTANCE * 2,
        constants::WORLD_BORDER_DISTANCE * 2, constants::WORLD_BORDER_DISTANCE * 2)) {
        unsigned int blockNum = (blockPosition.x - pos.x * constants::CHUNK_SIZE) + (blockPosition.y
            - pos.y * constants::CHUNK_SIZE) * constants::CHUNK_SIZE * constants::CHUNK_SIZE +
            (blockPosition.z - pos.z * constants::CHUNK_SIZE) * constants::CHUNK_SIZE;
        lightQueue.push(blockNum);
    }
    //add the light values from chunk borders to the light queue
    unsigned int blockNum = 0;
    for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
        for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[0]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) + 1;
            bool newLowestSkyLight = (currentskyLight > neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newLowestSkyLight) {
                lightQueue.push(blockNum);
            }
            blockNum++;
        }
    }
    blockNum = 0;
    for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
        for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[1]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) + 1;
            bool newLowestSkyLight = (currentskyLight > neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newLowestSkyLight) {
                lightQueue.push(blockNum);
            }
            blockNum++;
        }
        blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    }
    blockNum = 0;
    for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
        for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[2]).getSkyLight(
                blockNum + constants::CHUNK_SIZE - 1) + 1;
            bool newLowestSkyLight = (currentskyLight > neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newLowestSkyLight) {
                lightQueue.push(blockNum);
            }
            blockNum += constants::CHUNK_SIZE;
        }
    }
    blockNum = constants::CHUNK_SIZE - 1;
    for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
        for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[3]).getSkyLight(
                blockNum - constants::CHUNK_SIZE + 1) + 1;
            bool newLowestSkyLight = (currentskyLight > neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newLowestSkyLight) {
                lightQueue.push(blockNum);
            }
            blockNum += constants::CHUNK_SIZE;
        }
    }
    blockNum = constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
        for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[4]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) + 1;
            bool newLowestSkyLight = (currentskyLight > neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newLowestSkyLight) {
                lightQueue.push(blockNum);
            }
            blockNum++;
        }
        blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    }
    blockNum = constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
    for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
        for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[5]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) + 1;
            //propogate direct skylight down without reducing its intensity
            neighbouringskyLight -= (neighbouringskyLight == 16)
                * !(resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[5]).getBlock(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).dimsLight);
            bool newLowestSkyLight = (currentskyLight > neighbouringskyLight) * resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
            if (newLowestSkyLight) {
                lightQueue.push(blockNum);
            }
            blockNum++;
        }
    }
    // Propogate the decreased light values to the neighbouring blocks in the chunk
    for (char i = 0; i < 6; i++) {
        neighbouringChunksToBeRelit[i] = false;
    }
    while (!lightQueue.empty()) {
        lightQueue.pop();
        // Find the new value of the block
        unsigned int blockNum = lightQueue.front();
        unsigned char skyLight = chunk.getSkyLight(blockNum) - 1;
        unsigned char neighbouringSkyLights[6];
        unsigned char highestNeighbourSkyLight;
        if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[5];
            neighbouringSkyLights[5] = chunk.getSkyLight(neighbouringBlockNum);
            highestNeighbourSkyLight = neighbouringSkyLights[5];
        }
        else {
            neighbouringSkyLights[5] = worldChunks.at(neighbouringChunkPositions[5]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            highestNeighbourSkyLight = neighbouringSkyLights[5];
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[4];
            neighbouringSkyLights[4] = chunk.getSkyLight(neighbouringBlockNum);
            highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[4]);
        }
        else {
            neighbouringSkyLights[4] = worldChunks.at(neighbouringChunkPositions[4]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[4]);
        }
        if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[3];
            neighbouringSkyLights[5] = chunk.getSkyLight(neighbouringBlockNum);
            highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[3]);
        }
        else {
            neighbouringSkyLights[3] = worldChunks.at(neighbouringChunkPositions[3]).getSkyLight(
                blockNum - (constants::CHUNK_SIZE - 1));
            highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[3]);
        }
        if ((blockNum % constants::CHUNK_SIZE) >= 1) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[2];
            neighbouringSkyLights[2] = chunk.getSkyLight(neighbouringBlockNum);
            highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[2]);
        }
        else {
            neighbouringSkyLights[2] = worldChunks.at(neighbouringChunkPositions[2]).getSkyLight(
                blockNum + (constants::CHUNK_SIZE - 1));
            highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[2]);
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[1];
            neighbouringSkyLights[1] = chunk.getSkyLight(neighbouringBlockNum);
            highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[1]);
        }
        else {
            neighbouringSkyLights[1] = worldChunks.at(neighbouringChunkPositions[1]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[1]);
        }
        if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[0];
            neighbouringSkyLights[0] = chunk.getSkyLight(neighbouringBlockNum);
            highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[0]);
        }
        else {
            neighbouringSkyLights[0] = worldChunks.at(neighbouringChunkPositions[0]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[0]);
        }
        highestNeighbourSkyLight -= 1;
        if (highestNeighbourSkyLight > skyLight) {
            chunk.setSkyLight(blockNum, highestNeighbourSkyLight);
        }
        // Add the neighbouring blocks to the queue if they have a low enough sky light
        if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[5];
            if (neighbouringSkyLights[5] < skyLight) {
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            neighbouringSkyLights[5] = worldChunks.at(neighbouringChunkPositions[5]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            if (neighbouringSkyLights[5] < skyLight) {
                neighbouringChunksToBeRelit[5] =  true;
            }
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[4];
            if (neighbouringSkyLights[4] < skyLight) {
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            neighbouringSkyLights[4] = worldChunks.at(neighbouringChunkPositions[4]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            if (neighbouringSkyLights[4] < skyLight) {
                neighbouringChunksToBeRelit[4] =  true;
            }
        }
        if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[3];
            if (neighbouringSkyLights[3] < skyLight) {
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            neighbouringSkyLights[3] = worldChunks.at(neighbouringChunkPositions[3]).getSkyLight(
                blockNum - (constants::CHUNK_SIZE - 1));
            if (neighbouringSkyLights[3] < skyLight) {
                neighbouringChunksToBeRelit[3] =  true;
            }
        }
        if ((blockNum % constants::CHUNK_SIZE) >= 1) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[2];
            if (neighbouringSkyLights[2] < skyLight) {
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            neighbouringSkyLights[2] = worldChunks.at(neighbouringChunkPositions[2]).getSkyLight(
                blockNum + (constants::CHUNK_SIZE - 1));
            if (neighbouringSkyLights[2] < skyLight) {
                neighbouringChunksToBeRelit[2] =  true;
            }
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[1];
            if (neighbouringSkyLights[1] < skyLight) {
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            neighbouringSkyLights[1] = worldChunks.at(neighbouringChunkPositions[1]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            if (neighbouringSkyLights[1] < skyLight) {
                neighbouringChunksToBeRelit[1] =  true;
            }
        }
        if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[0];
            if (neighbouringSkyLights[0] < skyLight) {
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            neighbouringSkyLights[0] = worldChunks.at(neighbouringChunkPositions[0]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
            if (neighbouringSkyLights[0] < skyLight) {
                neighbouringChunksToBeRelit[0] =  true;
            }
        }
    }
    
    //(*m_worldInfo.numRelights)--;
    chunk.setSkyLightToBeUpToDate();
    //m_calculatingSkylight = false;
    // lock release
    // std::lock_guard<std::mutex> lock(chunk.accessingSkylightMtx);
    // m_accessingSkylightCV.notify_all();
}