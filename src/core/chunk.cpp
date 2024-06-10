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

#include "core/chunk.h"
#include "core/random.h"

#include <iostream>
#include <cmath>
#include <queue>
#include <thread>

#include "glm/glm.hpp"
#include "glm/gtc/noise.hpp"

std::mutex Chunk::s_checkingNeighbouringRelights;

//define the face positions constant:
const float Chunk::m_cubeTextureCoordinates[48] = { 0, 0,
													1, 0,
													1, 1,
													0, 1,
													
													0, 0,
													1, 0,
													1, 1,
													0, 1,
													
													0, 0,
													1, 0,
													1, 1,
													0, 1,
													
													0, 0,
													1, 0,
													1, 1,
													0, 1,
													
													0, 0,
													1, 0,
													1, 1,
													0, 1,
													
													0, 0,
													1, 0,
													1, 1,
													0, 1 };

const float Chunk::m_xTextureCoordinates[32] = { 0, 0,
												 1, 0,
												 1, 1,
												 0, 1,
												 
												 0, 0,
												 1, 0,
												 1, 1,
												 0, 1,
												 
												 0, 0,
												 1, 0,
												 1, 1,
												 0, 1,
												 
												 0, 0,
												 1, 0,
												 1, 1,
												 0, 1 };

const short Chunk::m_blockIdToTextureNum[48] = { 0, 0, 0, 0, 0, 0, //air
												 0, 0, 0, 0, 0, 0, //dirt
												 2, 2, 2, 2, 0, 1, //grass
												 3, 3, 3, 3, 3, 3, //stone
												 4, 4, 4, 4, 4, 4, //water
												 36, 36, 36, 36, 37, 37, //oak log
												 38, 38, 38, 38, 38, 38, //oak leaves
												 39, 39, 39, 39, 39, 39 //tall grass
												 };

const short Chunk::m_neighbouringBlocks[6] = { -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -constants::CHUNK_SIZE, -1, 1, constants::CHUNK_SIZE, (constants::CHUNK_SIZE * constants::CHUNK_SIZE) };

const short Chunk::m_neighbouringBlocksX[6] = { 0, 0, -1, 1, 0, 0 };

const short Chunk::m_neighbouringBlocksY[6] = { -1, 0, 0, 0, 0, 1 };

const short Chunk::m_neighbouringBlocksZ[6] = { 0, -1, 0, 0, 1, 0 };

const int Chunk::m_neighbouringChunkBlockOffsets[6] = { constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1), constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1), constants::CHUNK_SIZE - 1, -(constants::CHUNK_SIZE - 1), -(constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1)) };

const short Chunk::m_adjacentBlocksToFaceOffests[48] = { -1 - constants::CHUNK_SIZE, -constants::CHUNK_SIZE, -constants::CHUNK_SIZE + 1, 1, 1 + constants::CHUNK_SIZE, constants::CHUNK_SIZE, constants::CHUNK_SIZE - 1, -1,
														  1 - (constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE) - 1, -1, -1 + constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1, 1,
														  -constants::CHUNK_SIZE - (constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE) + constants::CHUNK_SIZE, constants::CHUNK_SIZE, constants::CHUNK_SIZE + constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE - constants::CHUNK_SIZE, -constants::CHUNK_SIZE,
														  constants::CHUNK_SIZE - (constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE) - constants::CHUNK_SIZE, -constants::CHUNK_SIZE, -constants::CHUNK_SIZE + constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE + constants::CHUNK_SIZE, constants::CHUNK_SIZE,
														  -1 - (constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE) + 1, 1, 1 + constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE - 1, -1,
														  -1 + constants::CHUNK_SIZE, constants::CHUNK_SIZE, constants::CHUNK_SIZE + 1, 1, 1 - constants::CHUNK_SIZE, -constants::CHUNK_SIZE, -constants::CHUNK_SIZE - 1, -1 };

const short Chunk::m_adjacentBlocksToFaceOffestsX[48] = { -1, 0, 1, 1, 1, 0, -1, -1,
														  1, 0, -1, -1, -1, 0, 1, 1,
														  0, 0, 0, 0, 0, 0, 0, 0,
														  0, 0, 0, 0, 0, 0, 0, 0,
														  -1, 0, 1, 1, 1, 0, -1, -1,
														  -1, 0, 1, 1, 1, 0, -1, -1 };

const short Chunk::m_adjacentBlocksToFaceOffestsY[48] = { 0, 0, 0, 0, 0, 0, 0, 0,
														  -1, -1, -1, 0, 1, 1, 1, 0,
														  -1, -1, -1, 0, 1, 1, 1, 0,
														  -1, -1, -1, 0, 1, 1, 1, 0,
														  -1, -1, -1, 0, 1, 1, 1, 0,
														  0, 0, 0, 0, 0, 0, 0, 0 };

const short Chunk::m_adjacentBlocksToFaceOffestsZ[48] = { -1, -1, -1, 0, 1, 1, 1, 0,
														  0, 0, 0, 0, 0, 0, 0, 0,
														  -1, 0, 1, 1, 1, 0, -1, -1,
														  1, 0, -1, -1, -1, 0, 1, 1,
														  0, 0, 0, 0, 0, 0, 0, 0,
														  1, 1, 1, 0, -1, -1, -1, 0 };

Chunk::Chunk(int x, int y, int z, WorldInfo wio) {
	inUse = true;
	m_singleBlockType = false;
	m_singleSkyLightVal = false;
	m_skyLightUpToDate = false;
	m_calculatingSkylight = false;
	m_worldInfo = wio;

	m_blocks = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
	m_skyLight = new unsigned char[(constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];

	m_position[0] = x;
	m_position[1] = y;
	m_position[2] = z;
	generateTerrain();
}

Chunk::Chunk(WorldInfo wio) {
	inUse = false;
	m_singleBlockType = false;
	m_singleSkyLightVal = false;
	m_skyLightUpToDate = false;
	m_calculatingSkylight = false;
	m_worldInfo = wio;

	m_blocks = new unsigned char[0];
	m_skyLight = new unsigned char[0];

	m_position[0] = 0;
	m_position[1] = 0;
	m_position[2] = 0;
}

Chunk::Chunk() {
	inUse = false;
	m_singleBlockType = false;
	m_singleSkyLightVal = false;
	m_skyLightUpToDate = false;
	m_calculatingSkylight = false;
	m_worldInfo = WorldInfo();

	m_blocks = new unsigned char[0];
	m_skyLight = new unsigned char[0];

	m_position[0] = 0;
	m_position[1] = 0;
	m_position[2] = 0;
}

void Chunk::recreate(int x, int y, int z) {
	inUse = true;
	m_singleBlockType = false;
	m_singleSkyLightVal = false;
	m_calculatingSkylight = false;
	delete[] m_blocks;
	delete[] m_skyLight;
	m_blocks = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
	m_skyLight = new unsigned char[(constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];

	m_position[0] = x;
	m_position[1] = y;
	m_position[2] = z;
	generateTerrain();
}

void Chunk::setWorldInfo(WorldInfo wio) {
	m_worldInfo = wio;
}

void Chunk::generateTerrain() {
	m_skyLightUpToDate = false;
	for (unsigned int blockNum = 0; blockNum < constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE; blockNum++) {
		m_blocks[blockNum] = 0;
		m_skyLight[blockNum >> 1] = 0;
	}

	//calculate coordinates of the chunk
	int chunkMinCoords[3];
	int chunkMaxCoords[3];
	for (unsigned char i = 0; i < 3; i++) {
		chunkMinCoords[i] = m_position[i] * constants::CHUNK_SIZE;
		chunkMaxCoords[i] = chunkMinCoords[i] + constants::CHUNK_SIZE;
	}

	m_singleBlockType = true;

	const int MAX_STRUCTURE_RADIUS = 3;
	const int HEIGHT_MAP_SIZE = constants::CHUNK_SIZE + MAX_STRUCTURE_RADIUS * 2;
	const int PV_noiseGridSize = HEIGHT_MAP_SIZE + 1;
	float PV_n[PV_noiseGridSize * PV_noiseGridSize * s_PV_NUM_OCTAVES]; //noise value for the octaves
	float PV_d[PV_noiseGridSize * PV_noiseGridSize * s_PV_NUM_OCTAVES]; //distance from simplex borders for the octaves
	float CONTINENTALNESS_n[HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE * s_CONTINENTALNESS_NUM_OCTAVES];
	float PVLOC_n[HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE * s_PVLOC_NUM_OCTAVES];
	float RIVERS_n[HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE * s_RIVERS_NUM_OCTAVES];
	float RIVER_BUMPS_n[HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE * s_RIVER_BUMPS_NUM_OCTAVES];
	m_PV_n = PV_n;
	m_PV_d = PV_d;
	m_CONTINENTALNESS_n = CONTINENTALNESS_n;
	m_PVLOC_n = PVLOC_n;
	m_RIVERS_n = RIVERS_n;
	m_RIVER_BUMPS_n = RIVER_BUMPS_n;

	calculateAllHeightMapNoise(chunkMinCoords[0] - MAX_STRUCTURE_RADIUS, chunkMinCoords[2] - MAX_STRUCTURE_RADIUS, HEIGHT_MAP_SIZE);

	int heightMap[HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE];
	// generateHeightMap(heightMap,
	// 				  chunkMinCoords[0] - MAX_STRUCTURE_RADIUS,
	// 				  chunkMinCoords[2] - MAX_STRUCTURE_RADIUS,
	// 				  HEIGHT_MAP_SIZE);

	int blockPos[3];
	unsigned int lastBlockTypeInChunk = 0;
	for (int z = -MAX_STRUCTURE_RADIUS; z < constants::CHUNK_SIZE + MAX_STRUCTURE_RADIUS; z++) {
		for (int x = -MAX_STRUCTURE_RADIUS; x < constants::CHUNK_SIZE + MAX_STRUCTURE_RADIUS; x++) {
			int height = sumNoisesAndCalculateHeight(chunkMinCoords[0] - MAX_STRUCTURE_RADIUS, chunkMinCoords[2] - MAX_STRUCTURE_RADIUS, x + MAX_STRUCTURE_RADIUS, z + MAX_STRUCTURE_RADIUS, HEIGHT_MAP_SIZE);
			heightMap[(z + MAX_STRUCTURE_RADIUS) * HEIGHT_MAP_SIZE + (x + MAX_STRUCTURE_RADIUS)] = height;
			
			if ((x >= 0) && (x < constants::CHUNK_SIZE) && (z >= 0) && (z < constants::CHUNK_SIZE)) {
				unsigned int blockNum = z * constants::CHUNK_SIZE + x;
				for (int y = chunkMinCoords[1]; y < chunkMaxCoords[1]; y++) {
					if (y > height) {
						if (y > 0) {
							//m_blocks[blockNum] = 0;
							m_skyLight[blockNum / 2] |= (15 << (4 * (blockNum % 2)));
						}
						else {
							m_blocks[blockNum] = 4;
							m_skyLight[blockNum / 2] |= ((15 + std::max(y - 1, -15)) << (4 * (blockNum % 2)));
						}
					}
					else if (y == height) {
						if (y < 0) {
							m_blocks[blockNum] = 1;
						}
						else {
							m_blocks[blockNum] = 2;
						}
					}
					else if (y > (height - 3)) {
						m_blocks[blockNum] = 3;
					}
					else {
						m_blocks[blockNum] = 3;
					}
					m_singleBlockType &= ((blockNum == 0) || (m_blocks[blockNum] == lastBlockTypeInChunk));
					lastBlockTypeInChunk = m_blocks[blockNum];
					blockNum += constants::CHUNK_SIZE * constants::CHUNK_SIZE;
				}
			}

			int worldX = x + chunkMinCoords[0];
			int worldZ = z + chunkMinCoords[2];

			//add trees
			if ((m_peaksAndValleysLocation > 0.23f) && (m_peaksAndValleysHeight < 120.0f) && (height >= 0) && (chunkMinCoords[1] < (height + 8)) && ((chunkMaxCoords[1]) > height)) {
				//convert the 2d block coordinate into a unique integer that can be used as the seed for the PRNG
				int blockNumberInWorld;
				if (worldZ > worldX) {
					blockNumberInWorld = ((worldZ + constants::WORLD_BORDER_DISTANCE) + 2) * (worldZ + constants::WORLD_BORDER_DISTANCE) - (worldX + constants::WORLD_BORDER_DISTANCE);
				}
				else {
					blockNumberInWorld = (worldX + constants::WORLD_BORDER_DISTANCE) * (worldX + constants::WORLD_BORDER_DISTANCE) + (worldZ + constants::WORLD_BORDER_DISTANCE);
				}
				int random = PCG_Hash32(blockNumberInWorld + m_worldInfo.seed) % 40u;
				if (random == 0) {
					bool nearbyTree = false;
					for (int checkZ = worldZ - 3; checkZ <= worldZ; checkZ++) {
						for (int checkX = worldX - 3; checkX <= worldX + 3; checkX++) {
							if (checkZ > checkX) {
								blockNumberInWorld = ((checkZ + constants::WORLD_BORDER_DISTANCE) + 2) * (checkZ + constants::WORLD_BORDER_DISTANCE) - (checkX + constants::WORLD_BORDER_DISTANCE);
							}
							else {
								blockNumberInWorld = (checkX + constants::WORLD_BORDER_DISTANCE) * (checkX + constants::WORLD_BORDER_DISTANCE) + (checkZ + constants::WORLD_BORDER_DISTANCE);
							}
							int random = PCG_Hash32(blockNumberInWorld + m_worldInfo.seed) % 40u;
							if ((random == 0) && (!((checkX == worldX) && (checkZ == worldZ)))) {
								nearbyTree = true;
								checkZ = worldZ + 1;
								break;
							}
						}
					}
					if (!nearbyTree) {
						//position has been selected for adding a tree
						//set the blocks for the tree
						int treeBasePos[3];
						int treeBlockPos[3];
						treeBasePos[1] = height + 1;
						treeBlockPos[1] = height;
						treeBasePos[0] = treeBlockPos[0] = worldX;
						treeBasePos[2] = treeBlockPos[2] = worldZ;
						int treeBlockNum = (treeBlockPos[0] + constants::BORDER_DISTANCE_U_B) % constants::CHUNK_SIZE + ((treeBlockPos[1] + constants::BORDER_DISTANCE_U_B) % constants::CHUNK_SIZE) * constants::CHUNK_SIZE * constants::CHUNK_SIZE + ((treeBlockPos[2] + constants::BORDER_DISTANCE_U_B) % constants::CHUNK_SIZE) * constants::CHUNK_SIZE;
						//build the trunk
						int trunkHeight = 3 + PCG_Hash32(blockNumberInWorld + m_worldInfo.seed) % 3;
						for (int logHeight = -1; logHeight < trunkHeight + 2; logHeight++) {
							//if the block is in the chunk
							if (((treeBlockPos[1] >= chunkMinCoords[1]) && (treeBlockPos[1] < chunkMaxCoords[1])) && ((treeBlockPos[0] >= chunkMinCoords[0]) && (treeBlockPos[0] < chunkMaxCoords[0])) && ((treeBlockPos[2] >= chunkMinCoords[2]) && (treeBlockPos[2] < chunkMaxCoords[2]))) {
								m_blocks[treeBlockNum] = 1 + (logHeight >= 0) * (4 + (logHeight >= trunkHeight));
								m_singleBlockType = false;
								m_skyLight[treeBlockNum / 2] &= (15 << (4 * !(treeBlockNum % 2))) | (0b11111111 * (logHeight >= trunkHeight));
							}
							treeBlockPos[1]++;
							treeBlockNum += constants::CHUNK_SIZE * constants::CHUNK_SIZE;
							treeBlockNum = treeBlockNum % (constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE);
						}
						//build the upper leaves
						treeBlockPos[0] -= 1;
						treeBlockPos[1] -= 2;
						for (unsigned char i = 0; i < 4; i++) {
							for (unsigned char ii = 0; ii < 2; ii++) {
								if (((treeBlockPos[1] >= chunkMinCoords[1]) && (treeBlockPos[1] < chunkMaxCoords[1])) && ((treeBlockPos[0] >= chunkMinCoords[0]) && (treeBlockPos[0] < chunkMaxCoords[0])) && ((treeBlockPos[2] >= chunkMinCoords[2]) && (treeBlockPos[2] < chunkMaxCoords[2]))) {
								treeBlockNum = (treeBlockPos[0] + constants::BORDER_DISTANCE_U_B) % constants::CHUNK_SIZE + ((treeBlockPos[1] + constants::BORDER_DISTANCE_U_B) % constants::CHUNK_SIZE) * constants::CHUNK_SIZE * constants::CHUNK_SIZE + ((treeBlockPos[2] + constants::BORDER_DISTANCE_U_B) % constants::CHUNK_SIZE) * constants::CHUNK_SIZE;
									m_blocks[treeBlockNum] = 6;
									m_singleBlockType = false;
								}
								treeBlockPos[1]++;
							}
							treeBlockPos[1] -= 2;
							treeBlockPos[0] += 1 + (i / 2) * -2;
							treeBlockPos[2] += 1 + ((i + 1) / 2) * -2;
						}
						//build the lower leaves
						treeBlockPos[0] += 4;
						treeBlockPos[1] -= 2;
						for (treeBlockPos[2] = treeBasePos[2] - 2; treeBlockPos[2] < treeBasePos[2] + 3; treeBlockPos[2]++) {
							for (treeBlockPos[0] = treeBasePos[0] - 2; treeBlockPos[0] < treeBasePos[0] + 3; treeBlockPos[0]++) {
								for (unsigned char ii = 0; ii < 2; ii++) {
									if (((treeBlockPos[1] >= chunkMinCoords[1]) && (treeBlockPos[1] < chunkMaxCoords[1])) && ((treeBlockPos[0] >= chunkMinCoords[0]) && (treeBlockPos[0] < chunkMaxCoords[0])) && ((treeBlockPos[2] >= chunkMinCoords[2]) && (treeBlockPos[2] < chunkMaxCoords[2]))) {
									treeBlockNum = (treeBlockPos[0] + constants::BORDER_DISTANCE_U_B) % constants::CHUNK_SIZE + ((treeBlockPos[1] + constants::BORDER_DISTANCE_U_B) % constants::CHUNK_SIZE) * constants::CHUNK_SIZE * constants::CHUNK_SIZE + ((treeBlockPos[2] + constants::BORDER_DISTANCE_U_B) % constants::CHUNK_SIZE) * constants::CHUNK_SIZE;
										if ((m_blocks[treeBlockNum] == 0) || (m_blocks[treeBlockNum] == 7)) {
											m_blocks[treeBlockNum] = 6;
											m_singleBlockType = false;
										}
									}
									treeBlockPos[1]++;
								}
								treeBlockPos[1] -= 2;
							}
						}
					}
				}
			}

			//add grass
			if ((x >= 0) && (x < constants::CHUNK_SIZE) && (z >= 0) && (z < constants::CHUNK_SIZE)
				&& (height >= 0) && (chunkMinCoords[1] < (height + 2)) && (chunkMaxCoords[1] > height)) {
				//convert the 2d block coordinate into a unique integer that can be used as the seed for the PRNG
				int blockNumberInWorld;
				if (worldZ > worldX) {
					blockNumberInWorld = ((worldZ + constants::WORLD_BORDER_DISTANCE) + 2) * (worldZ + constants::WORLD_BORDER_DISTANCE) - (worldX + constants::WORLD_BORDER_DISTANCE);
				}
				else {
					blockNumberInWorld = (worldX + constants::WORLD_BORDER_DISTANCE) * (worldX + constants::WORLD_BORDER_DISTANCE) + (worldZ + constants::WORLD_BORDER_DISTANCE);
				}
				int random = PCG_Hash32(blockNumberInWorld + m_worldInfo.seed) % 3u;
				if (random == 0) {
					int blockNum = x % constants::CHUNK_SIZE + (height + 1) % constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE + z * constants::CHUNK_SIZE;
					if (m_blocks[blockNum] == 0) {
						m_blocks[blockNum] = 7;
						m_singleBlockType = false;
					}
				}
			}
		}
	}



	//if the chunk is made up of a single block, compress it
	if (m_singleBlockType) {
		unsigned short blockType = m_blocks[0];
		delete[] m_blocks;
		m_blocks = new unsigned char[1];
		m_blocks[0] = blockType;
	}
}

//define world generation constants
const int Chunk::s_PV_NUM_OCTAVES = 5;
const int Chunk::s_CONTINENTALNESS_NUM_OCTAVES = 7;
const int Chunk::s_PVLOC_NUM_OCTAVES = 2;
const int Chunk::s_RIVERS_NUM_OCTAVES = 5;
const int Chunk::s_RIVER_BUMPS_NUM_OCTAVES = 2;
const float Chunk::s_PV_SCALE = 576.0f;
const float Chunk::s_PV_HEIGHT = 128.0f;
const float Chunk::s_RIVER_BUMPS_HEIGHT = 1.5f;

void Chunk::calculateAllHeightMapNoise(int minX, int minZ, int size) {
	//calculate the noise values for each position in the grid and for each octave for peaks and valleys
	const int PV_noiseGridSize = size + 1;
	for (int z = 0; z < PV_noiseGridSize; z++) {
		for (int x = 0; x < PV_noiseGridSize; x++) {
			int noiseGridIndex = z * PV_noiseGridSize + x;
			for (int octaveNum = 0; octaveNum < s_PV_NUM_OCTAVES; octaveNum++) {
				m_PV_n[noiseGridIndex + PV_noiseGridSize * PV_noiseGridSize * octaveNum] = 
					simplexNoise2d((minX + x + constants::WORLD_BORDER_DISTANCE) / (s_PV_SCALE / (float)(1 << octaveNum)),
								   (minZ + z + constants::WORLD_BORDER_DISTANCE) / (s_PV_SCALE / (float)(1 << octaveNum)),
								   m_PV_d + (noiseGridIndex + PV_noiseGridSize * PV_noiseGridSize * octaveNum));
			}
		}
	}
	
	// //calculate the noise values for each position in the grid and for each octave for smooth terrain
	// const int SMOOTH_NUM_OCTAVES = 3;
	// const float SMOOTH_HEIGHT = 2.0f;
	// float SMOOTH_n[size * size * SMOOTH_NUM_OCTAVES]; //noise value for the octaves
	// calculateFractalNoiseOctaves(SMOOTH_n, minX, minZ, size, SMOOTH_NUM_OCTAVES, 256.0f);

	calculateFractalNoiseOctaves(m_CONTINENTALNESS_n, minX, minZ, size, s_CONTINENTALNESS_NUM_OCTAVES, 2304.0f);
	calculateFractalNoiseOctaves(m_PVLOC_n, minX, minZ, size, s_PVLOC_NUM_OCTAVES, 768.0f);
	calculateFractalNoiseOctaves(m_RIVERS_n, minX, minZ, size, s_RIVERS_NUM_OCTAVES, 1600.0f);
	calculateFractalNoiseOctaves(m_RIVER_BUMPS_n, minX, minZ, size, s_RIVER_BUMPS_NUM_OCTAVES, 32.0f);
}

int Chunk::sumNoisesAndCalculateHeight(int minX, int minZ, int x, int z, int size) {
	//sum the peaks and valleys noises (including gradient trick)
	int PV_noiseGridSize = size + 1;
	int noiseGridIndex = z * PV_noiseGridSize + x;
	m_peaksAndValleysHeight = 0.0f;
	for (int octaveNum = 0; octaveNum < s_PV_NUM_OCTAVES; octaveNum++) {
		float gradx = 0.0f;
		float gradz = 0.0f;
		float tempGradx, tempGradz;
		//if the coordinates of the point are close to the edge of a simplex, calculate the gradient
		//at a point that is slightly offset, to avoid problems with the gradient near simplex edges
		const float BORDER_ERROR = 2.0f; //controls how close blocks have to be to the border to be recalculated
		if (m_PV_d[noiseGridIndex] < BORDER_ERROR / (s_PV_SCALE / (float)(1 << octaveNum))) {
			float distanceFromError;
			float d1, d2;
			float offset;
			int xDirections[4] = { 1, -1, 0, 0 };
			int zDirections[4] = { 0, 0, 1, -1 };
			float* gradDir = &tempGradx;
			for (int i = 0; i < 2; i++) {
				distanceFromError = 0.0f;
				offset = 0.0f;
				while (distanceFromError < BORDER_ERROR / (s_PV_SCALE / (float)(1 << octaveNum))) {
					offset += 0.25f;
					int direction = 0;
					while ((direction < 4) && (distanceFromError < BORDER_ERROR / (s_PV_SCALE / (float)(1 << octaveNum)))) {
						*gradDir = simplexNoise2d((minX + x + offset * xDirections[direction] + !i + constants::WORLD_BORDER_DISTANCE) / (s_PV_SCALE / (float)(1 << octaveNum)), (minZ + z + offset * zDirections[direction] + i + constants::WORLD_BORDER_DISTANCE) / (s_PV_SCALE / (float)(1 << octaveNum)), &d1)
						- simplexNoise2d((minX + x + offset * xDirections[direction] + constants::WORLD_BORDER_DISTANCE) / (s_PV_SCALE / (float)(1 << octaveNum)), (minZ + z + offset * zDirections[direction] + constants::WORLD_BORDER_DISTANCE) / (s_PV_SCALE / (float)(1 << octaveNum)), &d2);
						distanceFromError = std::min(d1, d2);
						direction++;
					}
				}
				gradDir = &tempGradz;
			}
			gradx += tempGradx;
			gradz += tempGradz;
		}
		else {
			gradx += m_PV_n[noiseGridIndex + PV_noiseGridSize * PV_noiseGridSize * octaveNum + 1] - m_PV_n[noiseGridIndex + PV_noiseGridSize * PV_noiseGridSize * octaveNum];
			gradz += m_PV_n[noiseGridIndex + PV_noiseGridSize * PV_noiseGridSize * octaveNum + PV_noiseGridSize] - m_PV_n[noiseGridIndex + PV_noiseGridSize * PV_noiseGridSize * octaveNum];
		}
		m_peaksAndValleysHeight += m_PV_n[noiseGridIndex + PV_noiseGridSize * PV_noiseGridSize * octaveNum]
			* (1.0f / (100.0f / std::pow(2.0f, (float)octaveNum / 1.2f) * (std::abs(gradx) + std::abs(gradz)) + 1.0f))
			* s_PV_HEIGHT / (float)(1 << octaveNum);
	}

	noiseGridIndex = z * size + x;

	// //sum the smooth terrain noises
	// float smoothHeight = 0.0f;
	// for (int octaveNum = 0; octaveNum < SMOOTH_NUM_OCTAVES; octaveNum++) {
	// 	smoothHeight += SMOOTH_n[noiseGridIndex + size * size * octaveNum]
	// 		* SMOOTH_HEIGHT / (float)(1 << octaveNum);
	// }

	//sum the continentalness terrain noises
	m_continentalness = 0.0f;
	for (int octaveNum = 0; octaveNum < s_CONTINENTALNESS_NUM_OCTAVES; octaveNum++) {
		m_continentalness += m_CONTINENTALNESS_n[noiseGridIndex + size * size * octaveNum]
			* 1.0f / (float)(1 << octaveNum);
	}

	//sum the peaks and valleys location terrain noises
	m_peaksAndValleysLocation = 0.0f;
	for (int octaveNum = 0; octaveNum < s_PVLOC_NUM_OCTAVES; octaveNum++) {
		m_peaksAndValleysLocation += m_PVLOC_n[noiseGridIndex + size * size * octaveNum]
			* 1.0f / (float)(1 << octaveNum);
	}

	//sum the rivers terrain noises
	m_riversNoise = 0.0f;
	for (int octaveNum = 0; octaveNum < s_RIVERS_NUM_OCTAVES; octaveNum++) {
		m_riversNoise += m_RIVERS_n[noiseGridIndex + size * size * octaveNum]
			* 1.0f / (float)(1 << octaveNum);
	}

	//sum the river bumps terrain noises
	float riverBumpsNoise = 0.0f;
	for (int octaveNum = 0; octaveNum < s_RIVER_BUMPS_NUM_OCTAVES; octaveNum++) {
		riverBumpsNoise += m_RIVER_BUMPS_n[noiseGridIndex + size * size * octaveNum]
			* s_RIVER_BUMPS_HEIGHT / (float)(1 << octaveNum);
	}

	//reduce continentalness slightly to increase ocean size
	m_continentalness = (m_continentalness - 0.3f);
	//calculate the height of the cliff noise
	const float cliffTop = -0.4f; //the original value of continentalness where the tops of the cliffs are
	const float cliffBase = -0.42f; //the original value of continentalness where the bases of the cliffs are
	const float cliffHeight = 0.5f; //the new value of continentalness that the tops of cliffs will be set to
	const float cliffDepth = -0.7f; //the new value of continentalness that the bases of cliffs will be set to
	float cliffContinentalness;
	//use the y = mx + c formula to transform the original continentalness value to the cliffs value
	if (m_continentalness > cliffTop) {
		cliffContinentalness = (1.0f - cliffHeight) / (1.0f - cliffTop) * (m_continentalness - cliffTop) + cliffHeight;
	}
	else if (m_continentalness < cliffBase) {
		cliffContinentalness = (-1.0f - cliffDepth) / (-1.0f - cliffBase) * (m_continentalness - cliffBase) + cliffDepth;
	}
	else {
		cliffContinentalness = (cliffHeight - cliffDepth) / (cliffTop - cliffBase) * (m_continentalness - cliffTop) + cliffHeight;
	}
	//calculate how much of the cliffs noise to use and how much of the original continentalness noise to use
	//this is done by reducing the cliffs near rivers and high peaks and valleys areas
	float cliffFactor = std::max(std::min(std::abs(m_riversNoise) / 1.5f - 0.1f, 0.4f - (m_peaksAndValleysLocation + 1.1f) / 2.5f), 0.0f) * 2.0f;
	//combine continentalness with the cliffs noise
	m_continentalness = m_continentalness * (1.0f - cliffFactor) + cliffContinentalness * cliffFactor;

	//calculate the height of the rivers
	//increse the noise by 1 to try to avoid crossections of two rivers
	m_riversNoise += 0.1f;
	//modify the river noise value to be closer to 0 (promotes wider river) near continentalness of -0.4 to create river mouths
	m_riversNoise = std::pow(std::abs(m_riversNoise), (1.55f - std::min(m_continentalness + 0.4f, 0.5f)) * 1.15f);
	//calculate the river errosion using the equation 1 / (nx - 1) + 1
	//this is the value that the rest of the terrain height will be multiplied by to create low terrain near rivers
	float riverErrosion = 1.0f / (-2.0f * std::abs(m_riversNoise) - 1.0f) + 1.0f;
	//calculate the value that determines where the extra bumps for the river bed will be added
	float invertedRiverErrosion = 1.0f - riverErrosion;
	float riverBumpsNoiseMultiplier1 = invertedRiverErrosion * invertedRiverErrosion;
	riverBumpsNoiseMultiplier1 *= riverBumpsNoiseMultiplier1;
	float riverBumpsNoiseMultiplier2 = riverBumpsNoiseMultiplier1 * riverBumpsNoiseMultiplier1;
	riverBumpsNoiseMultiplier2 *= riverBumpsNoiseMultiplier2;
	//reduce the multiplier near river mouths to give the look that the river is actually part of the ocean near river mouths
	riverBumpsNoiseMultiplier2 *= (1.1f - std::min(-m_continentalness + 0.4f, 1.0f)) * 0.9f;
	//calculate the heigt of the river using the equation m / (nx^p - 1) + 1
	float riversHeight = -6.0f / (1.0f + 1000000.0f * m_riversNoise * m_riversNoise * m_riversNoise) + riverBumpsNoise * riverBumpsNoiseMultiplier2;

	//scale the peaks and valleys location noise to be an S-shape and between the values of 0 and 1.4
	//using equation -1 / (mx^n + 1) + 1
	m_peaksAndValleysLocation = -1.5f / (1.4 * (m_peaksAndValleysLocation + 1.35f) * (m_peaksAndValleysLocation + 1.35f) + 1.0f) + 1.5f;
	//scale the peaks and valleys location to be higher near coasts so that mountains can still generate near coasts
	m_peaksAndValleysLocation *= (std::pow(std::abs(m_continentalness / 1.5f), 0.01f) * m_continentalness + 0.6f) / 1.6f;
	//scale the peaks and valleys height based on the peaks and valleys location noise
	m_peaksAndValleysHeight += 80.0f; //promotes all areas with high peaks and valleys to have a high y-value
	m_peaksAndValleysHeight *= m_peaksAndValleysLocation;

	// //calculate the height of the smooth noise by having it higher when peaks and valleys height is lower.
	// smoothHeight = (smoothHeight + 2.0f) * (2.0f - (peaksAndValleysLocation + std::abs(continentalness)) / 2.0f);

	//calculate the height of the terrain before rivers are added
	float nonRiverHeight = m_continentalness * 30.0f + 2.0f + m_peaksAndValleysHeight;// + smoothHeight;
	//calculate how much of the river errosion needs to be applied
	//without this step, rivers would not disapear at oceans
	float fac = (std::min(std::max(nonRiverHeight, -4.0f), 15.0f) + 4.0f) / 19.0f;
	riverErrosion = riverErrosion * fac + 1.0f - fac;
	//calculate how much of the river height needs to be applied
	//without this step, rivers would not disapear at oceans
	fac = (std::min(std::max(nonRiverHeight, -4.0f), 0.0f) + 4.0f) / 4.0f;
	riversHeight = riversHeight * fac;
	//add rivers to the terrain height
	return nonRiverHeight * riverErrosion + riversHeight;
}

void Chunk::calculateFractalNoiseOctaves(float* noiseArray, int minX, int minZ, int size, int numOctaves, float scale) {
	for (int z = 0; z < size; z++) {
		for (int x = 0; x < size; x++) {
			int noiseGridIndex = z * size + x;
			for (int octaveNum = 0; octaveNum < numOctaves; octaveNum++) {
				noiseArray[noiseGridIndex + size * size * octaveNum] = 
					simplexNoise2d((minX + x + constants::WORLD_BORDER_DISTANCE) / (scale / (float)(1 << octaveNum)),
								   (minZ + z + constants::WORLD_BORDER_DISTANCE) / (scale / (float)(1 << octaveNum)));
			}
		}
	}
}

void Chunk::findBlockCoordsInWorld(int* blockPos, unsigned int block) {
	int chunkCoords[3];
	getChunkPosition(chunkCoords);
	blockPos[0] = block % constants::CHUNK_SIZE + chunkCoords[0] * constants::CHUNK_SIZE;
	blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE) + chunkCoords[1] * constants::CHUNK_SIZE;
	blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE + chunkCoords[2] * constants::CHUNK_SIZE;
}

void Chunk::findBlockCoordsInChunk(float* blockPos, unsigned int block) {
	blockPos[0] = block % constants::CHUNK_SIZE;
	blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
	blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE;
}

void Chunk::findBlockCoordsInChunk(unsigned short* blockPos, unsigned int block) {
	blockPos[0] = block % constants::CHUNK_SIZE;
	blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
	blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE;
}

void Chunk::addFaceToMesh(float* vertices, unsigned int* numVertices, unsigned int* indices, unsigned int* numIndices, float* waterVertices, unsigned int* numWaterVertices, unsigned int* waterIndices, unsigned int* numWaterIndices, unsigned int block, short neighbouringBlock) {
	float blockPos[3];
	int neighbouringBlockPos[3];
	findBlockCoordsInChunk(blockPos, block);
	neighbouringBlockPos[0] = m_position[0] * constants::CHUNK_SIZE + blockPos[0] + m_neighbouringBlocksX[neighbouringBlock];
	neighbouringBlockPos[1] = m_position[1] * constants::CHUNK_SIZE + blockPos[1] + m_neighbouringBlocksY[neighbouringBlock];
	neighbouringBlockPos[2] = m_position[2] * constants::CHUNK_SIZE + blockPos[2] + m_neighbouringBlocksZ[neighbouringBlock];

	if (constants::cubeMesh[m_blocks[block]]) {
		short firstPositionIndex;
		short firstAdjacentBlockIndex;
		short textureNum;
		short neighbouringBlockCoordsOffset;
		switch (neighbouringBlock) {
			case 0:
				firstPositionIndex = 48;
				firstAdjacentBlockIndex = 0;
				textureNum = 4;
				neighbouringBlockCoordsOffset = 0;
				break;
			case 1:
				firstPositionIndex = 0;
				firstAdjacentBlockIndex = 8;
				textureNum = 2;
				neighbouringBlockCoordsOffset = 1;
				break;
			case 2:
				firstPositionIndex = 24;
				firstAdjacentBlockIndex = 16;
				textureNum = 0;
				neighbouringBlockCoordsOffset = 2;
				break;
			case 3:
				firstPositionIndex = 36;
				firstAdjacentBlockIndex = 24;
				textureNum = 1;
				neighbouringBlockCoordsOffset = 3;
				break;
			case 4:
				firstPositionIndex = 12;
				firstAdjacentBlockIndex = 32;
				textureNum = 3;
				neighbouringBlockCoordsOffset = 4;
				break;
			default: //5
				firstPositionIndex = 60;
				firstAdjacentBlockIndex = 40;
				textureNum = 5;
				neighbouringBlockCoordsOffset = 5;
				break;
		}

		if (m_blocks[block] == 4) {
			//TODO
			//find whether there is a block above the water to see if the water should be rendered as a full block or a surface block
			bool waterAbove;
			if (block >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
				int blockCoords[3];
				//wip
			}

			float texCoords[8];
			getTextureCoordinates(texCoords, m_blockIdToTextureNum[m_blocks[block] * 6 + textureNum]);
			for (short vertex = 0; vertex < 4; vertex++) {
				for (short element = 0; element < 3; element++) {
					waterVertices[*numWaterVertices] = constants::CUBE_FACE_POSITIONS[firstPositionIndex + vertex * 3 + element] + blockPos[element];
					(*numWaterVertices)++;
				}
				waterVertices[*numWaterVertices] = texCoords[vertex * 2];
				(*numWaterVertices)++;
				waterVertices[*numWaterVertices] = texCoords[vertex * 2 + 1];
				(*numWaterVertices)++;
				waterVertices[*numWaterVertices] = (1.0f / 16.0f) * (getWorldSkyLight(neighbouringBlockPos) + 1);
				(*numWaterVertices)++;
			}

			//index buffer
			int trueNumVertices = *numWaterVertices / 6;
			waterIndices[*numWaterIndices] = trueNumVertices - 4;
			(*numWaterIndices)++;
			waterIndices[*numWaterIndices] = trueNumVertices - 3;
			(*numWaterIndices)++;
			waterIndices[*numWaterIndices] = trueNumVertices - 2;
			(*numWaterIndices)++;
			waterIndices[*numWaterIndices] = trueNumVertices - 4;
			(*numWaterIndices)++;
			waterIndices[*numWaterIndices] = trueNumVertices - 2;
			(*numWaterIndices)++;
			waterIndices[*numWaterIndices] = trueNumVertices - 1;
			(*numWaterIndices)++;

			if (neighbouringBlock == 5) {
				firstPositionIndex = 48;
				firstAdjacentBlockIndex = 0;
				textureNum = 4;
				neighbouringBlockCoordsOffset = 0;
				blockPos[1]++;

				getTextureCoordinates(texCoords, m_blockIdToTextureNum[m_blocks[block] * 6 + textureNum]);
				for (short vertex = 0; vertex < 4; vertex++) {
					for (short element = 0; element < 3; element++) {
						waterVertices[*numWaterVertices] = constants::CUBE_FACE_POSITIONS[firstPositionIndex + vertex * 3 + element] + blockPos[element];
						(*numWaterVertices)++;
					}
					waterVertices[*numWaterVertices] = texCoords[vertex * 2];
					(*numWaterVertices)++;
					waterVertices[*numWaterVertices] = texCoords[vertex * 2 + 1];
					(*numWaterVertices)++;
					waterVertices[*numWaterVertices] = (1.0f / 16.0f) * (getWorldSkyLight(neighbouringBlockPos) + 1);
					(*numWaterVertices)++;
				}


				//index buffer
				trueNumVertices = *numWaterVertices / 6;
				waterIndices[*numWaterIndices] = trueNumVertices - 4;
				(*numWaterIndices)++;
				waterIndices[*numWaterIndices] = trueNumVertices - 3;
				(*numWaterIndices)++;
				waterIndices[*numWaterIndices] = trueNumVertices - 2;
				(*numWaterIndices)++;
				waterIndices[*numWaterIndices] = trueNumVertices - 4;
				(*numWaterIndices)++;
				waterIndices[*numWaterIndices] = trueNumVertices - 2;
				(*numWaterIndices)++;
				waterIndices[*numWaterIndices] = trueNumVertices - 1;
				(*numWaterIndices)++;
			}
		}
		else {
			float texCoords[8];
			getTextureCoordinates(texCoords, m_blockIdToTextureNum[m_blocks[block] * 6 + textureNum]);
			for (short vertex = 0; vertex < 4; vertex++) {
				for (short element = 0; element < 3; element++) {
					vertices[*numVertices] = constants::CUBE_FACE_POSITIONS[firstPositionIndex + vertex * 3 + element] + blockPos[element];
					(*numVertices)++;
				}
				vertices[*numVertices] = texCoords[vertex * 2];
				(*numVertices)++;
				vertices[*numVertices] = texCoords[vertex * 2 + 1];
				(*numVertices)++;
				vertices[*numVertices] = (1.0f / 16.0f) * (getWorldSkyLight(neighbouringBlockPos) + 1);
				(*numVertices)++;
			}

			//ambient occlusion
			short blockType;
			for (unsigned char adjacentBlockToFace = 0; adjacentBlockToFace < 8; adjacentBlockToFace++) {
				int blockCoords[3];
				blockCoords[0] = block % constants::CHUNK_SIZE;
				blockCoords[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
				blockCoords[2] = (block - blockCoords[1] * constants::CHUNK_SIZE * constants::CHUNK_SIZE) / constants::CHUNK_SIZE;
				blockCoords[0] += m_position[0] * constants::CHUNK_SIZE + m_neighbouringBlocksX[neighbouringBlockCoordsOffset] + m_adjacentBlocksToFaceOffestsX[firstAdjacentBlockIndex + adjacentBlockToFace];
				blockCoords[1] += m_position[1] * constants::CHUNK_SIZE + m_neighbouringBlocksY[neighbouringBlockCoordsOffset] + m_adjacentBlocksToFaceOffestsY[firstAdjacentBlockIndex + adjacentBlockToFace];
				blockCoords[2] += m_position[2] * constants::CHUNK_SIZE + m_neighbouringBlocksZ[neighbouringBlockCoordsOffset] + m_adjacentBlocksToFaceOffestsZ[firstAdjacentBlockIndex + adjacentBlockToFace];
				blockType = getWorldBlock(blockCoords);
				if (constants::castsShadows[blockType]) {
					if ((adjacentBlockToFace % 2) == 0) {
						vertices[*numVertices - (19 - adjacentBlockToFace / 2 * 6)] *= constants::shadowReceiveAmount[m_blocks[block]];
					}
					else {
						vertices[*numVertices - (19 - adjacentBlockToFace / 2 * 6)] *= constants::shadowReceiveAmount[m_blocks[block]];
						vertices[*numVertices - (19 - ((adjacentBlockToFace / 2 + 1) % 4) * 6)] *= constants::shadowReceiveAmount[m_blocks[block]];
					}
				}
			}

			//index buffer
			int trueNumVertices = *numVertices / 6;
			indices[*numIndices] = trueNumVertices - 4;
			(*numIndices)++;
			indices[*numIndices] = trueNumVertices - 3;
			(*numIndices)++;
			indices[*numIndices] = trueNumVertices - 2;
			(*numIndices)++;
			indices[*numIndices] = trueNumVertices - 4;
			(*numIndices)++;
			indices[*numIndices] = trueNumVertices - 2;
			(*numIndices)++;
			indices[*numIndices] = trueNumVertices - 1;
			(*numIndices)++;
		}
	} 
	else { //if the block is not a cube mesh
		//TODO:
		//make it so that it only adds the mesh if the mesh has not already been added (as only one "face" needs to be visible for them all to be visible
		float texCoords[8];
		short firstPositionIndex;

		for (unsigned char face = 0; face < 4; face++) {
			firstPositionIndex = face * 12;
			getTextureCoordinates(texCoords, m_blockIdToTextureNum[m_blocks[block] * 6 + face]);
			for (short vertex = 0; vertex < 4; vertex++) {
				for (short element = 0; element < 3; element++) {
					vertices[*numVertices] = constants::X_FACE_POSITIONS[firstPositionIndex + vertex * 3 + element] + blockPos[element];
					(*numVertices)++;
				}
				vertices[*numVertices] = texCoords[vertex * 2];
				(*numVertices)++;
				vertices[*numVertices] = texCoords[vertex * 2 + 1];
				(*numVertices)++;
				vertices[*numVertices] = (1.0f / 16.0f) * (getWorldSkyLight(neighbouringBlockPos) + 1);
				(*numVertices)++;
			}

			//TODO
			//lighting for X-shaped meshes

			//index buffer
			int trueNumVertices = *numVertices / 6;
			indices[*numIndices] = trueNumVertices - 4;
			(*numIndices)++;
			indices[*numIndices] = trueNumVertices - 3;
			(*numIndices)++;
			indices[*numIndices] = trueNumVertices - 2;
			(*numIndices)++;
			indices[*numIndices] = trueNumVertices - 4;
			(*numIndices)++;
			indices[*numIndices] = trueNumVertices - 2;
			(*numIndices)++;
			indices[*numIndices] = trueNumVertices - 1;
			(*numIndices)++;
		}
	}
}

void Chunk::buildMesh(float* vertices, unsigned int* numVertices, unsigned int* indices, unsigned int* numIndices, float* waterVertices, unsigned int* numWaterVertices, unsigned int* waterIndices, unsigned int* numWaterIndices, unsigned int* neighbouringChunkIndices) {
	if (!m_skyLightUpToDate) {
		bool neighbouringChunksToRelight[6];
		s_checkingNeighbouringRelights.lock();
		bool neighbourBeingRelit = true;
		while (neighbourBeingRelit) {
			neighbourBeingRelit = false;
			for (unsigned int i = 0; i < 6; i++) {
				neighbourBeingRelit |= m_worldInfo.worldChunks[neighbouringChunkIndices[i]].skyBeingRelit();
			}
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
		m_calculatingSkylight = true;
		clearSkyLight();
		calculateSkyLight(neighbouringChunkIndices, neighbouringChunksToRelight);
		m_calculatingSkylight = false;
	}

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

	int blockPos[3];
	int neighbouringBlockPos[3];
	int blockNum = 0;
	for (blockPos[1] = m_position[1] * constants::CHUNK_SIZE; blockPos[1] < (m_position[1] + 1) * constants::CHUNK_SIZE; blockPos[1]++) {
		for (blockPos[2] = m_position[2] * constants::CHUNK_SIZE; blockPos[2] < (m_position[2] + 1) * constants::CHUNK_SIZE; blockPos[2]++) {
			for (blockPos[0] = m_position[0] * constants::CHUNK_SIZE; blockPos[0] < (m_position[0] + 1) * constants::CHUNK_SIZE; blockPos[0]++) {
				if (m_blocks[blockNum] == 0) {
					blockNum++;
					continue;
				}
				if (m_blocks[blockNum] == 4) {
					for (short neighbouringBlock = 0; neighbouringBlock < 6; neighbouringBlock++) {
						neighbouringBlockPos[0] = blockPos[0] + m_neighbouringBlocksX[neighbouringBlock];
						neighbouringBlockPos[1] = blockPos[1] + m_neighbouringBlocksY[neighbouringBlock];
						neighbouringBlockPos[2] = blockPos[2] + m_neighbouringBlocksZ[neighbouringBlock];
						if (getWorldBlock(neighbouringBlockPos) != 4) {
							addFaceToMesh(vertices, numVertices, indices, numIndices, waterVertices, numWaterVertices, waterIndices, numWaterIndices, blockNum, neighbouringBlock);
						}
					}
				}
				else {
					for (short neighbouringBlock = 0; neighbouringBlock < 6; neighbouringBlock++) {
						neighbouringBlockPos[0] = blockPos[0] + m_neighbouringBlocksX[neighbouringBlock];
						neighbouringBlockPos[1] = blockPos[1] + m_neighbouringBlocksY[neighbouringBlock];
						neighbouringBlockPos[2] = blockPos[2] + m_neighbouringBlocksZ[neighbouringBlock];
						if (constants::transparent[getWorldBlock(neighbouringBlockPos)]) {
							addFaceToMesh(vertices, numVertices, indices, numIndices, waterVertices, numWaterVertices, waterIndices, numWaterIndices, blockNum, neighbouringBlock);
						}
					}
				}
				blockNum++;
			}
		}
	}

	//TODO:	make it so that chunks of a single block type get meshes without calling the new function
	if (m_singleBlockType) {
		unsigned char* temp = m_blocks;
		m_blocks = tempBlocks;
		delete[] temp;
	}
}

void Chunk::getTextureCoordinates(float* coords, short textureNum) {
	coords[0] = textureNum % 227 * 0.00439453125f + 0.000244140625f;
	coords[1] = 0.9956054688f - (textureNum / 227 * 0.00439453125f) + 0.000244140625f;
	coords[2] = coords[0] + 0.00390625f;
	coords[3] = coords[1];
	coords[4] = coords[2];
	coords[5] = coords[1] + 0.00390625f;
	coords[6] = coords[0];
	coords[7] = coords[5];
}

void Chunk::getChunkPosition(int* coordinates) const {
	for (char i = 0; i < 3; i++) {
		coordinates[i] = m_position[i];
	}
}

void Chunk::unload() {
	inUse = false;

	delete[] m_blocks;
	delete[] m_skyLight;
	m_blocks = new unsigned char[0];
	m_skyLight = new unsigned char[0];
}

unsigned int Chunk::getBlockNumber(unsigned int* blockCoords) {
	return blockCoords[0] + blockCoords[1] * constants::CHUNK_SIZE * constants::CHUNK_SIZE + blockCoords[2] * constants::CHUNK_SIZE;
}

void Chunk::setBlock(unsigned int block, unsigned short blockType) {
	if (m_singleBlockType) {
		unsigned char* tempBlocks = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
		for (unsigned int block = 0; block < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE); block++) {
			tempBlocks[block] = m_blocks[0];
		}
		unsigned char* temp = m_blocks;
		m_blocks = tempBlocks;
		m_singleBlockType = false;
		delete[] temp;
	}
	m_blocks[block] = blockType;
	setSkyLight(block, 0 * !(constants::castsShadows[blockType]));
}

unsigned char Chunk::getWorldBlock(int* blockCoords) {
	int chunkCoords[3];
	unsigned int blockPosInChunk[3];
	for (unsigned char i = 0; i < 3; i++) {
		chunkCoords[i] = floor(static_cast<float>(blockCoords[i]) / constants::CHUNK_SIZE);
		blockPosInChunk[i] = blockCoords[i] - chunkCoords[i] * constants::CHUNK_SIZE;
	}

	//get the chunk number
	int adjustedChunkCoords[3];
	for (unsigned char i = 0; i < 3; i++) {
		adjustedChunkCoords[i] = chunkCoords[i] - m_worldInfo.playerChunkPosition[i] + m_worldInfo.renderDistance;
	}

	unsigned int chunkNumber = adjustedChunkCoords[1] * m_worldInfo.renderDiameter * m_worldInfo.renderDiameter;
	chunkNumber += adjustedChunkCoords[2] * m_worldInfo.renderDiameter;
	chunkNumber += adjustedChunkCoords[0];


	unsigned int blockNumber = m_worldInfo.worldChunks[m_worldInfo.chunkArrayIndices[chunkNumber]].getBlockNumber(blockPosInChunk);

	return m_worldInfo.worldChunks[m_worldInfo.chunkArrayIndices[chunkNumber]].getBlock(blockNumber);
}

unsigned char Chunk::getWorldSkyLight(int* blockCoords) {
	int chunkCoords[3];
	unsigned int blockPosInChunk[3];
	for (unsigned char i = 0; i < 3; i++) {
		chunkCoords[i] = floor(static_cast<float>(blockCoords[i]) / constants::CHUNK_SIZE);
		blockPosInChunk[i] = blockCoords[i] - chunkCoords[i] * constants::CHUNK_SIZE;
	}

	//get the chunk number
	int adjustedChunkCoords[3];
	for (unsigned char i = 0; i < 3; i++) {
		adjustedChunkCoords[i] = chunkCoords[i] - m_worldInfo.playerChunkPosition[i] + m_worldInfo.renderDistance;
	}

	unsigned int chunkNumber = adjustedChunkCoords[1] * m_worldInfo.renderDiameter * m_worldInfo.renderDiameter;
	chunkNumber += adjustedChunkCoords[2] * m_worldInfo.renderDiameter;
	chunkNumber += adjustedChunkCoords[0];


	unsigned int blockNumber = m_worldInfo.worldChunks[m_worldInfo.chunkArrayIndices[chunkNumber]].getBlockNumber(blockPosInChunk);

	return m_worldInfo.worldChunks[m_worldInfo.chunkArrayIndices[chunkNumber]].getSkyLight(blockNumber);
}

void Chunk::clearSkyLight() {
	//reset all sky light values in the chunk to 0
	for (unsigned int blockNum = 0; blockNum < ((constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2); blockNum++) {
		m_skyLight[blockNum] = 0;
	}
}

void Chunk::calculateSkyLight(unsigned int* neighbouringChunkIndices, bool* neighbouringChunksToBeRelit) {
	if (!m_calculatingSkylight) {
		s_checkingNeighbouringRelights.lock();
		bool neighbourBeingRelit = true;
		while (neighbourBeingRelit) {
			neighbourBeingRelit = false;
			for (unsigned int i = 0; i < 6; i++) {
				neighbourBeingRelit |= m_worldInfo.worldChunks[neighbouringChunkIndices[i]].skyBeingRelit();
			}
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
	}
	s_checkingNeighbouringRelights.unlock();

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