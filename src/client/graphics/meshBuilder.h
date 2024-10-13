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
    unsigned int* m_numVertices;
    unsigned int* m_indices;
    unsigned int* m_numIndices;
    float* m_waterVertices;
    unsigned int* m_numWaterVertices;
    unsigned int* m_waterIndices;
    unsigned int* m_numWaterIndices;
    int m_chunkPosition[3];
    int m_chunkWorldCoords[3];

    static const short s_neighbouringBlocksX[7];
    static const short s_neighbouringBlocksY[7];
    static const short s_neighbouringBlocksZ[7];
    
    void getTextureCoordinates(float* coords, float* textureBox, const short textureNum);

    float getAmbientOcclusion(int* blockCoords, float* pointCoords, char direction);

    float getSmoothSkyLight(int* blockCoords, float* pointCoords, char direction);

    void addFaceToMesh(unsigned int block, unsigned char blockType, unsigned char faceNum);

    inline void findBlockCoordsInChunk(int* blockPos, unsigned int block) {
        blockPos[0] = block % constants::CHUNK_SIZE;
        blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE;
    }

public:
    MeshBuilder(Chunk& chunk, ServerWorld<true>& serverWorld, float* vertices, unsigned int*
        numVertices, unsigned int* indices, unsigned int* numIndices, float* waterVertices,
        unsigned int* numWaterVertices, unsigned int* waterIndices, unsigned int* numWaterIndices);

    void buildMesh();
};

}  // namespace client