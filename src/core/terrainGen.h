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

#include "chunk.h"

class TerrainGen {
private:
	static const int s_PV_NUM_OCTAVES;
	static const int s_CONTINENTALNESS_NUM_OCTAVES;
	static const int s_PVLOC_NUM_OCTAVES;
	static const int s_RIVERS_NUM_OCTAVES;
	static const int s_RIVER_BUMPS_NUM_OCTAVES;
	static const float s_PV_SCALE;
	static const float s_PV_HEIGHT;
	static const float s_RIVER_BUMPS_HEIGHT;

	float* m_PV_n;
	float* m_PV_d;
	float* m_CONTINENTALNESS_n;
	float* m_PVLOC_n;
	float* m_RIVERS_n;
	float* m_RIVER_BUMPS_n;

	float m_peaksAndValleysHeight;
	float m_continentalness;
	float m_peaksAndValleysLocation;
	float m_riversNoise;

	static void calculateFractalNoiseOctaves(float* noiseArray, int minX, int minZ, int size, int numOctaves, float scale);

    void calculateAllHeightMapNoise(int minX, int minZ, int size);

	int sumNoisesAndCalculateHeight(int minX, int minZ, int noiseX, int noiseZ, int size);
public:
	void generateTerrain(Chunk& chunk, unsigned long long seed);
};