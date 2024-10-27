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
#include "core/constants.h"
#include "core/resourcePack.h"
#include "core/serverWorld.h"

namespace client {

class MeshBuilder {
private:
    Chunk& m_chunk;
    ServerWorld<true>& m_serverWorld;
    float* m_vertices;
    uint32_t* m_numVertices;
    uint32_t* m_indices;
    uint32_t* m_numIndices;
    float* m_waterVertices;
    uint32_t* m_numWaterVertices;
    uint32_t* m_waterIndices;
    uint32_t* m_numWaterIndices;
    int m_chunkPosition[3];
    int m_chunkWorldCoords[3];

    static const int16_t s_neighbouringBlocksX[7];
    static const int16_t s_neighbouringBlocksY[7];
    static const int16_t s_neighbouringBlocksZ[7];
    
    void getTextureCoordinates(float* coords, float* textureBox, const int16_t textureNum);

    float getAmbientOcclusion(int* blockCoords, float* pointCoords, int8_t direction);

    float getSmoothSkyLight(int* blockCoords, float* pointCoords, int8_t direction);

    float getSmoothBlockLight(int* blockCoords, float* pointCoords, int8_t direction);

    void addFaceToMesh(uint32_t block, uint8_t blockType, uint8_t faceNum);

    inline void findBlockCoordsInChunk(int* blockPos, uint32_t block) {
        blockPos[0] = block % constants::CHUNK_SIZE;
        blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE;
    }

public:
    MeshBuilder(Chunk& chunk, ServerWorld<true>& serverWorld, float* vertices, uint32_t*
        numVertices, uint32_t* indices, uint32_t* numIndices, float* waterVertices,
        uint32_t* numWaterVertices, uint32_t* waterIndices, uint32_t* numWaterIndices);

    void buildMesh();
};

}  // namespace client