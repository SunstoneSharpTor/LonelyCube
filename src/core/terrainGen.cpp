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

#include "core/terrainGen.h"

#include <cmath>

#include "core/random.h"

//define world generation constants
const int TerrainGen::s_PV_NUM_OCTAVES = 5;
const int TerrainGen::s_CONTINENTALNESS_NUM_OCTAVES = 7;
const int TerrainGen::s_PVLOC_NUM_OCTAVES = 2;
const int TerrainGen::s_RIVERS_NUM_OCTAVES = 5;
const int TerrainGen::s_RIVER_BUMPS_NUM_OCTAVES = 2;
const float TerrainGen::s_PV_SCALE = 576.0f;
const float TerrainGen::s_PV_HEIGHT = 128.0f;
const float TerrainGen::s_RIVER_BUMPS_HEIGHT = 1.5f;

void TerrainGen::calculateFractalNoiseOctaves(float* noiseArray, int minX, int minZ, int size, int numOctaves, float scale) {
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

void TerrainGen::calculateAllHeightMapNoise(int minX, int minZ, int size) {
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

	calculateFractalNoiseOctaves(m_CONTINENTALNESS_n, minX, minZ, size, s_CONTINENTALNESS_NUM_OCTAVES, 2304.0f);
	calculateFractalNoiseOctaves(m_PVLOC_n, minX, minZ, size, s_PVLOC_NUM_OCTAVES, 768.0f);
	calculateFractalNoiseOctaves(m_RIVERS_n, minX, minZ, size, s_RIVERS_NUM_OCTAVES, 1600.0f);
	calculateFractalNoiseOctaves(m_RIVER_BUMPS_n, minX, minZ, size, s_RIVER_BUMPS_NUM_OCTAVES, 32.0f);
}

int TerrainGen::sumNoisesAndCalculateHeight(int minX, int minZ, int x, int z, int size) {
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

void TerrainGen::generateTerrain(Chunk& chunk, unsigned long long seed) {
	chunk.setSkyLightToBeOutdated();
    chunk.clearBlocksAndLight();

	//calculate coordinates of the chunk
    int chunkPosition[3];
	int chunkMinCoords[3];
	int chunkMaxCoords[3];
    chunk.getChunkPosition(chunkPosition);
	for (unsigned char i = 0; i < 3; i++) {
		chunkMinCoords[i] = chunkPosition[i] * constants::CHUNK_SIZE;
		chunkMaxCoords[i] = chunkMinCoords[i] + constants::CHUNK_SIZE;
	}

	chunk.setSingleBlockType(true);

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
                            chunk.setSkyLight(blockNum, 15);
						}
						else {
                            chunk.setBlockUnchecked(blockNum, 4);
                            chunk.setSkyLight(blockNum, 15 + std::max(y - 1, -15));
						}
					}
					else if (y == height) {
						if (y < 0) {
							chunk.setBlockUnchecked(blockNum, 1);
						}
						else {
							chunk.setBlockUnchecked(blockNum, 2);
						}
					}
					else if (y > (height - 3)) {
						chunk.setBlockUnchecked(blockNum, 3);
					}
					else {
						chunk.setBlockUnchecked(blockNum, 3);
					}
					chunk.setSingleBlockType(chunk.isSingleBlockType()
						&& ((blockNum == 0) || (chunk.getBlockUnchecked(blockNum) == lastBlockTypeInChunk)));
					lastBlockTypeInChunk = chunk.getBlockUnchecked(blockNum);
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
				int random = PCG_Hash32(blockNumberInWorld + seed) % 40u;
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
							int random = PCG_Hash32(blockNumberInWorld + seed) % 40u;
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
						int trunkHeight = 3 + PCG_Hash32(blockNumberInWorld + seed) % 3;
						for (int logHeight = -1; logHeight < trunkHeight + 2; logHeight++) {
							//if the block is in the chunk
							if (((treeBlockPos[1] >= chunkMinCoords[1]) && (treeBlockPos[1] < chunkMaxCoords[1])) && ((treeBlockPos[0] >= chunkMinCoords[0]) && (treeBlockPos[0] < chunkMaxCoords[0])) && ((treeBlockPos[2] >= chunkMinCoords[2]) && (treeBlockPos[2] < chunkMaxCoords[2]))) {
								chunk.setBlockUnchecked(treeBlockNum, 1 + (logHeight >= 0) * (4 + (logHeight >= trunkHeight)));
								chunk.setSingleBlockType(false);
								chunk.setSkyLight(treeBlockNum, (15 << (4 * !(treeBlockNum % 2))) | (15 * (logHeight >= trunkHeight)));
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
									chunk.setBlockUnchecked(treeBlockNum, 6);
									chunk.setSingleBlockType(false);
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
										if ((chunk.getBlockUnchecked(treeBlockNum) == 0) || (chunk.getBlockUnchecked(treeBlockNum) == 7)) {
											chunk.setBlockUnchecked(treeBlockNum, 6);
											chunk.setSingleBlockType(false);
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
				int random = PCG_Hash32(blockNumberInWorld + seed) % 3u;
				if (random == 0) {
					int blockNum = x % constants::CHUNK_SIZE + (height + 1) % constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE + z * constants::CHUNK_SIZE;
					if (chunk.getBlockUnchecked(blockNum) == 0) {
						chunk.setBlockUnchecked(blockNum, 7);
						chunk.setSingleBlockType(false);
					}
				}
			}
		}
	}

    chunk.compressBlocks();
}