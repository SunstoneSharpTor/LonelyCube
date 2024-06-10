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

#include <thread>

#include "core/chunk.h"
#include "core/position.h"

void Lighting::calculateSkyLight(Chunk& chunk, std::unordered_map<Position, Chunk> worldChunks, bool* neighbouringChunksToBeRelit) {
    int* chunkPosition;
    chunk.getChunkPosition(chunkPosition);
    Position neibouringChunkPositions[6] = { Position(chunkPosition[0], chunkPosition[1] - 1, chunkPosition[2]),
                                             Position(chunkPosition[0], chunkPosition[1], chunkPosition[2] - 1),
                                             Position(chunkPosition[0] - 1, chunkPosition[1], chunkPosition[2]),
                                             Position(chunkPosition[0] + 1, chunkPosition[1], chunkPosition[2]),
                                             Position(chunkPosition[0], chunkPosition[1], chunkPosition[2] + 1),
                                             Position(chunkPosition[0], chunkPosition[1] + 1, chunkPosition[2]) };
	if (!chunk.skyBeingRelit()) {
		chunk.s_checkingNeighbouringRelights.lock();
		bool neighbourBeingRelit = true;
		while (neighbourBeingRelit) {
			neighbourBeingRelit = false;
			for (unsigned int i = 0; i < 6; i++) {
				neighbourBeingRelit |= worldChunks[neibouringChunkPositions[i]].skyBeingRelit();
			}
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
	}
	chunk.s_checkingNeighbouringRelights.unlock();

	(*m_worldInfo.numRelights)++;

	//TODO:	make it so that chunks of a single block type get meshes without calling the new function
	unsigned char* tempBlocks = nullptr;
	if (m_singleBlockType) {
		tempBlocks = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
		for (unsigned int block = 0; block < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE); block++) {
			tempBlocks[block] = m_blocks[0];
		}
		unsigned char* temp = m_blocks;
		m_blocks = tempBlocks;
		tempBlocks = temp;
	}

	std::queue<unsigned int> lightQueue;
	//add the light values from chunk borders to the light queue
	unsigned int blockNum = 0;
	for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
		for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
			char currentskyLight = getSkyLight(blockNum);
			char neighbouringskyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[0]].getSkyLight(
				blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
			bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[m_blocks[blockNum]];
			if (newHighestSkyLight) {
				setSkyLight(blockNum, neighbouringskyLight);
				lightQueue.push(blockNum);
			}
			blockNum++;
		}
	}
	blockNum = 0;
	for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
		for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
			char currentskyLight = getSkyLight(blockNum);
			char neighbouringskyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[1]].getSkyLight(
				blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
			bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[m_blocks[blockNum]];
			if (newHighestSkyLight) {
				setSkyLight(blockNum, neighbouringskyLight);
				lightQueue.push(blockNum);
			}
			blockNum++;
		}
		blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
	}
	blockNum = 0;
	for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
		for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
			char currentskyLight = getSkyLight(blockNum);
			char neighbouringskyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[2]].getSkyLight(
				blockNum + constants::CHUNK_SIZE - 1) - 1;
			bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[m_blocks[blockNum]];
			if (newHighestSkyLight) {
				setSkyLight(blockNum, neighbouringskyLight);
				lightQueue.push(blockNum);
			}
			blockNum += constants::CHUNK_SIZE;
		}
	}
	blockNum = constants::CHUNK_SIZE - 1;
	for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
		for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
			char currentskyLight = getSkyLight(blockNum);
			char neighbouringskyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[3]].getSkyLight(
				blockNum - constants::CHUNK_SIZE + 1) - 1;
			bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[m_blocks[blockNum]];
			if (newHighestSkyLight) {
				setSkyLight(blockNum, neighbouringskyLight);
				lightQueue.push(blockNum);
			}
			blockNum += constants::CHUNK_SIZE;
		}
	}
	blockNum = constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
	for (unsigned int y = 0; y < constants::CHUNK_SIZE; y++) {
		for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
			char currentskyLight = getSkyLight(blockNum);
			char neighbouringskyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[4]].getSkyLight(
				blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
			bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[m_blocks[blockNum]];
			if (newHighestSkyLight) {
				setSkyLight(blockNum, neighbouringskyLight);
				lightQueue.push(blockNum);
			}
			blockNum++;
		}
		blockNum += constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
	}
	blockNum = constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1);
	for (unsigned int z = 0; z < constants::CHUNK_SIZE; z++) {
		for (unsigned int x = 0; x < constants::CHUNK_SIZE; x++) {
			char currentskyLight = getSkyLight(blockNum);
			char neighbouringskyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[5]].getSkyLight(
				blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) - 1;
			//propogate direct skylight down without reducing its intensity
			neighbouringskyLight += (neighbouringskyLight == 14)
				* !(constants::dimsLight[m_worldInfo.worldChunks[neighbouringChunkIndices[5]].getBlock(
				blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))]);
			bool newHighestSkyLight = (currentskyLight < neighbouringskyLight) * !constants::castsShadows[m_blocks[blockNum]];
			if (newHighestSkyLight) {
				setSkyLight(blockNum, neighbouringskyLight);
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
		unsigned char skyLight = getSkyLight(blockNum) - 1;
		lightQueue.pop();
		if (blockNum < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
			unsigned int neighbouringBlockNum = blockNum + m_neighbouringBlocks[5];
			unsigned char neighbouringskyLight = getSkyLight(neighbouringBlockNum);
			bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[m_blocks[neighbouringBlockNum]];
			if (newHighestSkyLight) {
				setSkyLight(neighbouringBlockNum, skyLight);
				lightQueue.push(neighbouringBlockNum);
			}
		}
		else {
			bool transparrent = constants::transparent[m_worldInfo.worldChunks[neighbouringChunkIndices[5]].getBlock(
				blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))];
			bool newHighestSkyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[5]].getSkyLight(
				blockNum - constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
			if (transparrent && newHighestSkyLight) {
				neighbouringChunksToBeRelit[5] = true;
			}
		}
		if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) < (constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
			unsigned int neighbouringBlockNum = blockNum + m_neighbouringBlocks[4];
			unsigned char neighbouringskyLight = getSkyLight(neighbouringBlockNum);
			bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[m_blocks[neighbouringBlockNum]];
			if (newHighestSkyLight) {
				setSkyLight(neighbouringBlockNum, skyLight);
				lightQueue.push(neighbouringBlockNum);
			}
		}
		else {
			bool transparrent = constants::transparent[m_worldInfo.worldChunks[neighbouringChunkIndices[4]].getBlock(
				blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))];
			bool newHighestSkyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[4]].getSkyLight(
				blockNum - constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
			if (transparrent && newHighestSkyLight) {
				neighbouringChunksToBeRelit[4] = true;
			}
		}
		if ((blockNum % constants::CHUNK_SIZE) < (constants::CHUNK_SIZE - 1)) {
			unsigned int neighbouringBlockNum = blockNum + m_neighbouringBlocks[3];
			unsigned char neighbouringskyLight = getSkyLight(neighbouringBlockNum);
			bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[m_blocks[neighbouringBlockNum]];
			if (newHighestSkyLight) {
				setSkyLight(neighbouringBlockNum, skyLight);
				lightQueue.push(neighbouringBlockNum);
			}
		}
		else {
			bool transparrent = constants::transparent[m_worldInfo.worldChunks[neighbouringChunkIndices[3]].getBlock(
				blockNum - (constants::CHUNK_SIZE - 1))];
			bool newHighestSkyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[3]].getSkyLight(
				blockNum - (constants::CHUNK_SIZE - 1)) < skyLight;
			if (transparrent && newHighestSkyLight) {
				neighbouringChunksToBeRelit[3] = true;
			}
		}
		if ((blockNum % constants::CHUNK_SIZE) >= 1) {
			unsigned int neighbouringBlockNum = blockNum + m_neighbouringBlocks[2];
			unsigned char neighbouringskyLight = getSkyLight(neighbouringBlockNum);
			bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[m_blocks[neighbouringBlockNum]];
			if (newHighestSkyLight) {
				setSkyLight(neighbouringBlockNum, skyLight);
				lightQueue.push(neighbouringBlockNum);
			}
		}
		else {
			bool transparrent = constants::transparent[m_worldInfo.worldChunks[neighbouringChunkIndices[2]].getBlock(
				blockNum + (constants::CHUNK_SIZE - 1))];
			bool newHighestSkyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[2]].getSkyLight(
				blockNum + (constants::CHUNK_SIZE - 1)) < skyLight;
			if (transparrent && newHighestSkyLight) {
				neighbouringChunksToBeRelit[2] = true;
			}
		}
		if ((blockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) >= constants::CHUNK_SIZE) {
			unsigned int neighbouringBlockNum = blockNum + m_neighbouringBlocks[1];
			unsigned char neighbouringskyLight = getSkyLight(neighbouringBlockNum);
			bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[m_blocks[neighbouringBlockNum]];
			if (newHighestSkyLight) {
				setSkyLight(neighbouringBlockNum, skyLight);
				lightQueue.push(neighbouringBlockNum);
			}
		}
		else {
			bool transparrent = constants::transparent[m_worldInfo.worldChunks[neighbouringChunkIndices[1]].getBlock(
				blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))];
			bool newHighestSkyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[1]].getSkyLight(
				blockNum + constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
			if (transparrent && newHighestSkyLight) {
				neighbouringChunksToBeRelit[1] = true;
			}
		}
		if (blockNum >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE)) {
			unsigned int neighbouringBlockNum = blockNum + m_neighbouringBlocks[0];
			skyLight += (skyLight == 14) * !(constants::dimsLight[m_blocks[neighbouringBlockNum]]);
			unsigned char neighbouringskyLight = getSkyLight(neighbouringBlockNum);
			bool newHighestSkyLight = (neighbouringskyLight < skyLight) * !constants::castsShadows[m_blocks[neighbouringBlockNum]];
			if (newHighestSkyLight) {
				setSkyLight(neighbouringBlockNum, skyLight);
				lightQueue.push(neighbouringBlockNum);
			}
		}
		else {
			bool dimsLight = constants::dimsLight[m_worldInfo.worldChunks[neighbouringChunkIndices[0]].getBlock(
				blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))];
			skyLight += (skyLight == 14) * !(dimsLight);
			bool transparrent = constants::transparent[m_worldInfo.worldChunks[neighbouringChunkIndices[0]].getBlock(
				blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))];
			bool newHighestSkyLight = m_worldInfo.worldChunks[neighbouringChunkIndices[0]].getSkyLight(
				blockNum + constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) < skyLight;
			if (transparrent && newHighestSkyLight) {
				neighbouringChunksToBeRelit[0] = true;
			}
		}
	}

	//TODO:	make it so that chunks of a single block type get meshes without calling the new function
	if (m_singleBlockType) {
		unsigned char* temp = m_blocks;
		m_blocks = tempBlocks;
		delete[] temp;
	}

	
	(*m_worldInfo.numRelights)--;
	m_skyLightUpToDate = true;
	m_calculatingSkylight = false;
	// lock release
	std::lock_guard<std::mutex> lock(m_accessingSkylightMtx);
	m_accessingSkylightCV.notify_all();
}