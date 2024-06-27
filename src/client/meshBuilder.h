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

#pragma once

#include <unordered_map>

#include "core/chunk.h"
#include "core/constants.h"

namespace client {

class MeshBuilder {
private:
    Chunk m_chunk;

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

    void getTextureCoordinates(float* coords, short textureNum);

    void addFaceToMesh(float* vertices, unsigned int* numVertices,
        unsigned int* indices, unsigned int* numIndices, float* waterVertices,
        unsigned int* numWaterVertices, unsigned int* waterIndices,
        unsigned int* numWaterIndices, unsigned int block, short neighbouringBlock);

    inline void findBlockCoordsInChunk(float* blockPos, unsigned int block) {
        blockPos[0] = block % constants::CHUNK_SIZE;
        blockPos[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
        blockPos[2] = block / constants::CHUNK_SIZE % constants::CHUNK_SIZE;
    }

public:
    MeshBuilder(const Chunk& chunk) : m_chunk(chunk) {}

    void buildMesh(float* vertices, unsigned int* numVertices, unsigned int* indices,
        unsigned int* numIndices, float* waterVertices, unsigned int* numWaterVertices,
        unsigned int* waterIndices, unsigned int* numWaterIndices);
};

}  // namespace client