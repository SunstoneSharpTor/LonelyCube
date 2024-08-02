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

#include "core/lighting.h"

#include "core/pch.h"

#include "core/chunk.h"
#include "core/position.h"

void Lighting::calculateSkyLight(Position pos, std::unordered_map<Position, Chunk>& worldChunks, bool* neighbouringChunksToBeRelit) {
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

    //(*m_worldInfo.numRelights)++;

    std::queue<unsigned int> lightQueue;
    //add the light values from chunk borders to the light queue
    unsigned int blockNum = 0;
    for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
        for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
            signed char currentskyLight = chunk.getSkyLight(blockNum);
            signed char neighbouringskyLight = worldChunks[neighbouringChunkPositions[0]].getSkyLight(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[chunk.getBlock(blockNum)];
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
            signed char neighbouringskyLight = worldChunks[neighbouringChunkPositions[1]].getSkyLight(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[chunk.getBlock(blockNum)];
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
            signed char neighbouringskyLight = worldChunks[neighbouringChunkPositions[2]].getSkyLight(
                blockNum + constants::CHUNK_SIZE - 1) - 1;
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[chunk.getBlock(blockNum)];
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
            signed char neighbouringskyLight = worldChunks[neighbouringChunkPositions[3]].getSkyLight(
                blockNum - constants::CHUNK_SIZE + 1) - 1;
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[chunk.getBlock(blockNum)];
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
            signed char neighbouringskyLight = worldChunks[neighbouringChunkPositions[4]].getSkyLight(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[chunk.getBlock(blockNum)];
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
            signed char neighbouringskyLight = worldChunks[neighbouringChunkPositions[5]].getSkyLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
            //propogate direct skylight down without reducing its intensity
            neighbouringskyLight += (neighbouringskyLight == 14)
                * !(constants::dimsLight[worldChunks[neighbouringChunkPositions[5]].getBlock(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))]);
            bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[chunk.getBlock(blockNum)];
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
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[chunk.getBlock(neighbouringBlockNum)];
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool transparrent = constants::transparent[worldChunks[neighbouringChunkPositions[5]].getBlock(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))];
            bool newHighestSkyLight = worldChunks[neighbouringChunkPositions[5]].getSkyLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparrent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[5] = true;
            }
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[4];
            unsigned char neighbouringskyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[chunk.getBlock(neighbouringBlockNum)];
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool transparrent = constants::transparent[worldChunks[neighbouringChunkPositions[4]].getBlock(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))];
            bool newHighestSkyLight = worldChunks[neighbouringChunkPositions[4]].getSkyLight(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparrent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[4] = true;
            }
        }
        if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[3];
            unsigned char neighbouringskyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[chunk.getBlock(neighbouringBlockNum)];
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool transparrent = constants::transparent[worldChunks[neighbouringChunkPositions[3]].getBlock(
                blockNum - (constants::CHUNK_SIZE - 1))];
            bool newHighestSkyLight = worldChunks[neighbouringChunkPositions[3]].getSkyLight(
                blockNum - (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparrent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[3] = true;
            }
        }
        if ((blockNum % constants::CHUNK_SIZE) >= 1) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[2];
            unsigned char neighbouringskyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[chunk.getBlock(neighbouringBlockNum)];
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool transparrent = constants::transparent[worldChunks[neighbouringChunkPositions[2]].getBlock(
                blockNum + (constants::CHUNK_SIZE - 1))];
            bool newHighestSkyLight = worldChunks[neighbouringChunkPositions[2]].getSkyLight(
                blockNum + (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparrent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[2] = true;
            }
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[1];
            unsigned char neighbouringskyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[chunk.getBlock(neighbouringBlockNum)];
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool transparrent = constants::transparent[worldChunks[neighbouringChunkPositions[1]].getBlock(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))];
            bool newHighestSkyLight = worldChunks[neighbouringChunkPositions[1]].getSkyLight(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparrent && newHighestSkyLight) {
                neighbouringChunksToBeRelit[1] = true;
            }
        }
        if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[0];
            skyLight += (skyLight == 14) * !(constants::dimsLight[chunk.getBlock(neighbouringBlockNum)]);
            unsigned char neighbouringskyLight = chunk.getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[chunk.getBlock(neighbouringBlockNum)];
            if (newHighestSkyLight) {
                chunk.setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(neighbouringBlockNum);
            }
        }
        else {
            bool dimsLight = constants::dimsLight[worldChunks[neighbouringChunkPositions[0]].getBlock(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))];
            skyLight += (skyLight == 14) * !(dimsLight);
            bool transparrent = constants::transparent[worldChunks[neighbouringChunkPositions[0]].getBlock(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))];
            bool newHighestSkyLight = worldChunks[neighbouringChunkPositions[0]].getSkyLight(
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