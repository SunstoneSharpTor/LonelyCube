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

namespace client {

class MeshBuilder {
private:
    Chunk& m_chunk;
    ResourcePack& m_resourcePack;
    float* m_vertices;
    unsigned int* m_numVertices;
    unsigned int* m_indices;
    unsigned int* m_numIndices;
    float* m_waterVertices;
    unsigned int* m_numWaterVertices;
    unsigned int* m_waterIndices;
    unsigned int* m_numWaterIndices;

    static const short s_neighbouringBlocks[6];
    static const short s_neighbouringBlocksX[6];
    static const short s_neighbouringBlocksY[6];
    static const short s_neighbouringBlocksZ[6];
    static const float s_cubeTextureCoordinates[48];
    static const float s_xTextureCoordinates[32];
    static const short s_blockIdToTextureNum[48];
    static const short s_adjacentBlocksToFaceOffests[48];
    static const short s_adjacentBlocksToFaceOffestsX[48];
    static const short s_adjacentBlocksToFaceOffestsY[48];
    static const short s_adjacentBlocksToFaceOffestsZ[48];

    void getTextureCoordinates(float* coords, const short textureNum);

    void addFaceToMesh(unsigned int block, unsigned char faceNum);

    inline void findBlockCoordsInChunk(int* blockPos, unsigned int block) {
        blockPos[0] = block % constants::CHUNK_SIZE;
        blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE;
    }

public:
    MeshBuilder(Chunk& chunk, ResourcePack& resourcePack, float* vertices, unsigned int*
        numVertices, unsigned int* indices, unsigned int* numIndices, float* waterVertices,
        unsigned int* numWaterVertices, unsigned int* waterIndices, unsigned int* numWaterIndices)
        : m_chunk(chunk), m_resourcePack(resourcePack), m_vertices(vertices),
        m_numVertices(numVertices), m_indices(indices), m_numIndices(numIndices),
        m_waterVertices(waterVertices), m_numWaterVertices(numWaterVertices),
        m_waterIndices(waterIndices), m_numWaterIndices(numWaterIndices) {}

    void buildMesh();
};

}  // namespace client