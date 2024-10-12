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

void Lighting::propagateSkyLight(Position updatedBlockPos, std::unordered_map<Position, Chunk>&
    worldChunks, std::vector<Position>& chunksToRemesh, ResourcePack& resourcePack) {
    std::queue<Position> lightQueue;
    lightQueue.push(updatedBlockPos);
    lightQueue.push(updatedBlockPos + Position(-1, 0, 0));
    lightQueue.push(updatedBlockPos + Position(1, 0, 0));
    lightQueue.push(updatedBlockPos + Position(0, -1, 0));
    lightQueue.push(updatedBlockPos + Position(0, 1, 0));
    lightQueue.push(updatedBlockPos + Position(0, 0, -1));
    lightQueue.push(updatedBlockPos + Position(0, 0, 1));
    std::array <Chunk*, 7> chunks;
    Position chunkPosition;
    chunkPosition.x = (updatedBlockPos.x + (updatedBlockPos.x < 0)) / constants::CHUNK_SIZE -
        (updatedBlockPos.x < 0);
    chunkPosition.y = (updatedBlockPos.y + (updatedBlockPos.y < 0)) / constants::CHUNK_SIZE -
        (updatedBlockPos.y < 0);
    chunkPosition.z = (updatedBlockPos.z + (updatedBlockPos.z < 0)) / constants::CHUNK_SIZE -
        (updatedBlockPos.z < 0);
    if (!worldChunks.contains(chunkPosition)) {
        return;
    }
    Position lastChunkPos = chunkPosition + Position(1, 0, 0);
    while (!lightQueue.empty()) {
        Position blockPos = lightQueue.front();
        lightQueue.pop();
        chunkPosition.x = (blockPos.x + (blockPos.x < 0)) / constants::CHUNK_SIZE -
            (blockPos.x < 0);
        chunkPosition.y = (blockPos.y + (blockPos.y < 0)) / constants::CHUNK_SIZE -
            (blockPos.y < 0);
        chunkPosition.z = (blockPos.z + (blockPos.z < 0)) / constants::CHUNK_SIZE -
            (blockPos.z < 0);
        if (chunkPosition != lastChunkPos) {
            //check that the chunk has its neighbours loaded so that it can be lit
            Position neighbouringChunkPositions[6];
            bool neighbouringChunksLoaded = true;
            for (char ii = 0; ii < 6; ii++) {
                neighbouringChunkPositions[ii] = chunkPosition + s_neighbouringChunkOffets[ii];
                if (!worldChunks.contains(neighbouringChunkPositions[ii])) {
                    neighbouringChunksLoaded = false;
                    break;
                }
            }
            //if the chunk's neighbours aren't loaded, remove the chunk as it cannot be lit correctly
            if (!neighbouringChunksLoaded) {
                worldChunks.at(chunkPosition).setSkyLightToBeOutdated();
                continue;
            }

            chunks[6] = &worldChunks.at(chunkPosition);
            chunks[0] = &worldChunks.at(chunkPosition + Position(0, -1, 0));
            chunks[1] = &worldChunks.at(chunkPosition + Position(0, 0, -1));
            chunks[2] = &worldChunks.at(chunkPosition + Position(-1, 0, 0));
            chunks[3] = &worldChunks.at(chunkPosition + Position(1, 0, 0));
            chunks[4] = &worldChunks.at(chunkPosition + Position(0, 0, 1));
            chunks[5] = &worldChunks.at(chunkPosition + Position(0, 1, 0));
        }
        unsigned int blockNum = blockPos.x - chunkPosition.x * constants::CHUNK_SIZE
            + (blockPos.y - chunkPosition.y * constants::CHUNK_SIZE) * constants::CHUNK_SIZE
            * constants::CHUNK_SIZE + (blockPos.z - chunkPosition.z * constants::CHUNK_SIZE)
            * constants::CHUNK_SIZE;
        unsigned char skyLight = chunks[6]->getSkyLight(blockNum);
        skyLight -= 1 * skyLight > 0;
        if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[5];
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (0, 1, 0));
            }
        }
        else {
            unsigned int neighbouringBlockNum = blockNum - constants::CHUNK_SIZE *
                constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
            bool transparent = resourcePack.getBlockData(chunks[5]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[5]->getSkyLight(neighbouringBlockNum) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[5]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (0, 1, 0));
            }
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[4];
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (0, 0, 1));
            }
        }
        else {
            unsigned int neighbouringBlockNum = blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
            bool transparent = resourcePack.getBlockData(chunks[4]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[4]->getSkyLight(neighbouringBlockNum) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[4]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (0, 0, 1));
            }
        }
        if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[3];
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (1, 0, 0));
            }
        }
        else {
            unsigned int neighbouringBlockNum =  blockNum - (constants::CHUNK_SIZE - 1);
            bool transparent = resourcePack.getBlockData(chunks[3]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[3]->getSkyLight(
                blockNum - (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[3]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (1, 0, 0));
            }
        }
        if ((blockNum % constants::CHUNK_SIZE) >= 1) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[2];
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (1, 0, 0));
            }
        }
        else {
            unsigned int neighbouringBlockNum = blockNum + (constants::CHUNK_SIZE - 1);
            bool transparent = resourcePack.getBlockData(chunks[2]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[2]->getSkyLight(neighbouringBlockNum) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[2]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (1, 0, 0));
            }
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[1];
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (0, 0, 1));
            }
        }
        else {
            unsigned int neighbouringBlockNum = blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
            bool transparent = resourcePack.getBlockData(chunks[1]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[1]->getSkyLight(neighbouringBlockNum) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[1]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (0, 0, 1));
            }
        }
        if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[0];
            skyLight += (skyLight == 14) * !(resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).dimsLight);
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (0, 1, 0));
            }
        }
        else {
            unsigned int neighbouringBlockNum = blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
            bool dimsLight = resourcePack.getBlockData(chunks[0]->getBlock(neighbouringBlockNum)).dimsLight;
            skyLight += (skyLight == 14) * !(dimsLight);
            bool transparent = resourcePack.getBlockData(chunks[0]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[0]->getSkyLight(neighbouringBlockNum) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[0]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (0, 1, 0));
            }
        }
    }
}

void Lighting::propagateSkyDarkness(Position updatedBlockPos, std::unordered_map<Position, Chunk>&
    worldChunks, std::vector<Position>& chunksToRemesh, ResourcePack& resourcePack) {
    std::queue<Position> lightQueue;
    lightQueue.push(updatedBlockPos);
    std::array <Chunk*, 7> chunks;
    Position chunkPosition;
    chunkPosition.x = (updatedBlockPos.x + (updatedBlockPos.x < 0)) / constants::CHUNK_SIZE -
        (updatedBlockPos.x < 0);
    chunkPosition.y = (updatedBlockPos.y + (updatedBlockPos.y < 0)) / constants::CHUNK_SIZE -
        (updatedBlockPos.y < 0);
    chunkPosition.z = (updatedBlockPos.z + (updatedBlockPos.z < 0)) / constants::CHUNK_SIZE -
        (updatedBlockPos.z < 0);
    if (!worldChunks.contains(chunkPosition)) {
        return;
    }
    Position lastChunkPos = chunkPosition + Position(1, 0, 0);
    while (!lightQueue.empty()) {
        Position blockPos = lightQueue.front();
        lightQueue.pop();
        chunkPosition.x = (blockPos.x + (blockPos.x < 0)) / constants::CHUNK_SIZE -
            (blockPos.x < 0);
        chunkPosition.y = (blockPos.y + (blockPos.y < 0)) / constants::CHUNK_SIZE -
            (blockPos.y < 0);
        chunkPosition.z = (blockPos.z + (blockPos.z < 0)) / constants::CHUNK_SIZE -
            (blockPos.z < 0);
        if (chunkPosition != lastChunkPos) {
            //check that the chunk has its neighbours loaded so that it can be lit
            Position neighbouringChunkPositions[6];
            bool neighbouringChunksLoaded = true;
            for (char ii = 0; ii < 6; ii++) {
                neighbouringChunkPositions[ii] = chunkPosition + s_neighbouringChunkOffets[ii];
                if (!worldChunks.contains(neighbouringChunkPositions[ii])) {
                    neighbouringChunksLoaded = false;
                    break;
                }
            }
            //if the chunk's neighbours aren't loaded, remove the chunk as it cannot be lit correctly
            if (!neighbouringChunksLoaded) {
                worldChunks.at(chunkPosition).setSkyLightToBeOutdated();
                continue;
            }

            chunks[6] = &worldChunks.at(chunkPosition);
            chunks[0] = &worldChunks.at(chunkPosition + Position(0, -1, 0));
            chunks[1] = &worldChunks.at(chunkPosition + Position(0, 0, -1));
            chunks[2] = &worldChunks.at(chunkPosition + Position(-1, 0, 0));
            chunks[3] = &worldChunks.at(chunkPosition + Position(1, 0, 0));
            chunks[4] = &worldChunks.at(chunkPosition + Position(0, 0, 1));
            chunks[5] = &worldChunks.at(chunkPosition + Position(0, 1, 0));
        }
        unsigned int blockNum = blockPos.x - chunkPosition.x * constants::CHUNK_SIZE
            + (blockPos.y - chunkPosition.y * constants::CHUNK_SIZE) * constants::CHUNK_SIZE
            * constants::CHUNK_SIZE + (blockPos.z - chunkPosition.z * constants::CHUNK_SIZE)
            * constants::CHUNK_SIZE;
        unsigned char skyLight = chunks[6]->getSkyLight(blockNum);
        skyLight -= 1 * skyLight > 0;
        if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[5];
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (0, 1, 0));
            }
        }
        else {
            unsigned int neighbouringBlockNum = blockNum - constants::CHUNK_SIZE *
                constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
            bool transparent = resourcePack.getBlockData(chunks[5]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[5]->getSkyLight(neighbouringBlockNum) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[5]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (0, 1, 0));
            }
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[4];
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (0, 0, 1));
            }
        }
        else {
            unsigned int neighbouringBlockNum = blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
            bool transparent = resourcePack.getBlockData(chunks[4]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[4]->getSkyLight(neighbouringBlockNum) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[4]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (0, 0, 1));
            }
        }
        if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[3];
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (1, 0, 0));
            }
        }
        else {
            unsigned int neighbouringBlockNum =  blockNum - (constants::CHUNK_SIZE - 1);
            bool transparent = resourcePack.getBlockData(chunks[3]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[3]->getSkyLight(
                blockNum - (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[3]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos + Position (1, 0, 0));
            }
        }
        if ((blockNum % constants::CHUNK_SIZE) >= 1) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[2];
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (1, 0, 0));
            }
        }
        else {
            unsigned int neighbouringBlockNum = blockNum + (constants::CHUNK_SIZE - 1);
            bool transparent = resourcePack.getBlockData(chunks[2]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[2]->getSkyLight(neighbouringBlockNum) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[2]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (1, 0, 0));
            }
        }
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[1];
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (0, 0, 1));
            }
        }
        else {
            unsigned int neighbouringBlockNum = blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
            bool transparent = resourcePack.getBlockData(chunks[1]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[1]->getSkyLight(neighbouringBlockNum) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[1]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (0, 0, 1));
            }
        }
        if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
            unsigned int neighbouringBlockNum = blockNum + chunks[6]->neighbouringBlocks[0];
            skyLight += (skyLight == 14) * !(resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).dimsLight);
            unsigned char neighbouringskyLight = chunks[6]->getSkyLight(neighbouringBlockNum);
            bool newHighestSkyLight = (neighbouringskyLight < skyLight) * resourcePack.getBlockData(chunks[6]->getBlock(neighbouringBlockNum)).transparent;
            if (newHighestSkyLight) {
                chunks[6]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (0, 1, 0));
            }
        }
        else {
            unsigned int neighbouringBlockNum = blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
            bool dimsLight = resourcePack.getBlockData(chunks[0]->getBlock(neighbouringBlockNum)).dimsLight;
            skyLight += (skyLight == 14) * !(dimsLight);
            bool transparent = resourcePack.getBlockData(chunks[0]->getBlock(neighbouringBlockNum)).transparent;
            bool newHighestSkyLight = chunks[0]->getSkyLight(neighbouringBlockNum) < skyLight;
            if (transparent && newHighestSkyLight) {
                chunks[0]->setSkyLight(neighbouringBlockNum, skyLight);
                lightQueue.push(blockPos - Position (0, 1, 0));
            }
        }
    }
}

void Lighting::propagateSkyLight(Position pos, std::unordered_map<Position, Chunk>& worldChunks,
    bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, ResourcePack& resourcePack, unsigned
    int modifiedBlock) {
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
        unsigned char skyLight = chunk.getSkyLight(blockNum);
        skyLight -= 1 * skyLight > 0;
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
            bool transparent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[5]).getBlock(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[5]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparent && newHighestSkyLight) {
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
            bool transparent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[4]).getBlock(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[4]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparent && newHighestSkyLight) {
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
            bool transparent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[3]).getBlock(
                blockNum - (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[3]).getSkyLight(
                blockNum - (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparent && newHighestSkyLight) {
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
            bool transparent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[2]).getBlock(
                blockNum + (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[2]).getSkyLight(
                blockNum + (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparent && newHighestSkyLight) {
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
            bool transparent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[1]).getBlock(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[1]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparent && newHighestSkyLight) {
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
            bool transparent = resourcePack.getBlockData(worldChunks.at(neighbouringChunkPositions[0]).getBlock(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))).transparent;
            bool newHighestSkyLight = worldChunks.at(neighbouringChunkPositions[0]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
            if (transparent && newHighestSkyLight) {
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
    bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, ResourcePack& resourcePack, unsigned
    int modifiedBlock) {
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
    // Add the the updated block to the light queue if it has been provided
    if (modifiedBlock < constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE) {
        lightQueue.push(modifiedBlock);
        chunk.setSkyLight(modifiedBlock, 15);
    }
    else {
        //add the light values from chunk borders to the light queue
        unsigned int blockNum = 0;
        for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
            for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
                signed char currentskyLight = chunk.getSkyLight(blockNum);
                signed char neighbouringskyLight = worldChunks.at(neighbouringChunkPositions[0]).getSkyLight(
                    blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
                if (currentskyLight >= neighbouringskyLight) {
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
                    blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
                if (currentskyLight >= neighbouringskyLight) {
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
                    blockNum + constants::CHUNK_SIZE - 1);
                if (currentskyLight >= neighbouringskyLight) {
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
                    blockNum - constants::CHUNK_SIZE + 1);
                if (currentskyLight >= neighbouringskyLight) {
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
                    blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
                if (currentskyLight >= neighbouringskyLight) {
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
                    blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
                if (currentskyLight >= neighbouringskyLight) {
                    lightQueue.push(blockNum);
                }
                blockNum++;
            }
        }
    }
    // Propogate the decreased light values to the neighbouring blocks in the chunk
    for (char i = 0; i < 6; i++) {
        neighbouringChunksToBeRelit[i] = false;
    }
    while (!lightQueue.empty()) {
        // Find the new value of the block
        unsigned int blockNum = lightQueue.front();
        unsigned char skyLight = chunk.getSkyLight(blockNum);
        lightQueue.pop();
        unsigned char neighbouringSkyLights[6];
        if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[5];
            neighbouringSkyLights[5] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[5] = worldChunks.at(neighbouringChunkPositions[5]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
        }
        unsigned char highestNeighbourSkyLight = neighbouringSkyLights[5];
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[4];
            neighbouringSkyLights[4] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[4] = worldChunks.at(neighbouringChunkPositions[4]).getSkyLight(
                blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
        }
        highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[4]);
        if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[3];
            neighbouringSkyLights[3] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[3] = worldChunks.at(neighbouringChunkPositions[3]).getSkyLight(
                blockNum - (constants::CHUNK_SIZE - 1));
        }
        highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[3]);
        if ((blockNum % constants::CHUNK_SIZE) >= 1) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[2];
            neighbouringSkyLights[2] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[2] = worldChunks.at(neighbouringChunkPositions[2]).getSkyLight(
                blockNum + (constants::CHUNK_SIZE - 1));
        }
        highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[2]);
        if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[1];
            neighbouringSkyLights[1] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[1] = worldChunks.at(neighbouringChunkPositions[1]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
        }
        highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[1]);
        if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
            unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[0];
            neighbouringSkyLights[0] = chunk.getSkyLight(neighbouringBlockNum);
        }
        else {
            neighbouringSkyLights[0] = worldChunks.at(neighbouringChunkPositions[0]).getSkyLight(
                blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1));
        }
        highestNeighbourSkyLight = std::max(highestNeighbourSkyLight, neighbouringSkyLights[0]);
        bool skyAccess = neighbouringSkyLights[5] == 15 && !resourcePack.getBlockData(chunk.getBlock(blockNum)).dimsLight;
        highestNeighbourSkyLight -= !skyAccess && highestNeighbourSkyLight > 0;
        highestNeighbourSkyLight *= resourcePack.getBlockData(chunk.getBlock(blockNum)).transparent;
        if (highestNeighbourSkyLight < skyLight) {
            chunk.setSkyLight(blockNum, highestNeighbourSkyLight);
            // Add the neighbouring blocks to the queue if they have a low enough sky light
            highestNeighbourSkyLight += highestNeighbourSkyLight == 0;
            if (neighbouringSkyLights[5] < skyLight && neighbouringSkyLights[5] >= highestNeighbourSkyLight) {
                if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
                    unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[5];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[5] =  true;
                }
            }
            if (neighbouringSkyLights[4] < skyLight && neighbouringSkyLights[4] >= highestNeighbourSkyLight) {
                if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
                    unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[4];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[4] =  true;
                }
            }
            if (neighbouringSkyLights[3] < skyLight && neighbouringSkyLights[3] >= highestNeighbourSkyLight) {
                if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
                    unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[3];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[3] =  true;
                }
            }
            if (neighbouringSkyLights[2] < skyLight && neighbouringSkyLights[2] >= highestNeighbourSkyLight) {
                if ((blockNum % constants::CHUNK_SIZE) >= 1) {
                    unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[2];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[2] =  true;
                }
            }
            if (neighbouringSkyLights[1] < skyLight && neighbouringSkyLights[1] >= highestNeighbourSkyLight) {
                if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
                    unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[1];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[1] =  true;
                }
            }
            if ((neighbouringSkyLights[0] < skyLight || skyLight == 15) && neighbouringSkyLights[0] >= highestNeighbourSkyLight) {
                if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
                    unsigned int neighbouringBlockNum = blockNum + chunk.neighbouringBlocks[0];
                    lightQueue.push(neighbouringBlockNum);
                }
                else {
                    neighbouringChunksToBeRelit[0] =  true;
                }
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