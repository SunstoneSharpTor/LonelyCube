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

#include "chunk.h"

namespace lonelycube {

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

    static const float s_cliffTop; //the original value of continentalness where the tops of the cliffs are
    static const float s_cliffBase; //the original value of continentalness where the bases of the cliffs are
    static const float s_cliffHeight; //the new value of continentalness that the tops of cliffs will be set to
    static const float s_cliffDepth; //the new value of continentalness that the bases of cliffs will be set to

    float* m_PV_n;
    float* m_PV_d;
    float* m_CONTINENTALNESS_n;
    float* m_PVLOC_n;
    float* m_RIVERS_n;
    float* m_RIVER_BUMPS_n;

    // Data about a particular column in the world
    float m_atCliffBase;
    float m_bumpsNoise;
    float m_continentalness;
    float m_cliffContinentalness;
    float m_cliffFactor;
    float m_height;
    float m_nonRiverHeight;
    float m_peaksAndValleysLocation;
    float m_preCliffContinentalness;
    float m_peaksAndValleysHeight;
    float m_riverErrosion;
    float m_riversHeight;
    float m_riversNoise;

    static void calculateFractalNoiseOctaves(float* noiseArray, int minX, int minZ, int size, int numOctaves, float scale);

    void calculateAllHeightMapNoise(int minX, int minZ, int size);

    int sumNoisesAndCalculateHeight(int minX, int minZ, int noiseX, int noiseZ, int size);
public:
    void generateTerrain(Chunk& chunk, uint64_t seed);
};

}  // namespace lonelycube
