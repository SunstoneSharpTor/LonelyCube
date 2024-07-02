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

#include "client/meshBuilder.h"

#include <iostream>
#include <unordered_map>

#include "core/chunk.h"
#include "core/constants.h"

namespace client{

//define the face positions constant:
const float MeshBuilder::s_cubeTextureCoordinates[48] = { 0, 0,
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

const float MeshBuilder::s_xTextureCoordinates[32] = { 0, 0,
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

const short MeshBuilder::s_blockIdToTextureNum[48] = { 0, 0, 0, 0, 0, 0, //air
                                                 0, 0, 0, 0, 0, 0, //dirt
                                                 2, 2, 2, 2, 0, 1, //grass
                                                 3, 3, 3, 3, 3, 3, //stone
                                                 4, 4, 4, 4, 4, 4, //water
                                                 36, 36, 36, 36, 37, 37, //oak log
                                                 38, 38, 38, 38, 38, 38, //oak leaves
                                                 39, 39, 39, 39, 39, 39 //tall grass
                                                 };

const short MeshBuilder::s_neighbouringBlocks[6] = { -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -constants::CHUNK_SIZE, -1, 1, constants::CHUNK_SIZE, (constants::CHUNK_SIZE * constants::CHUNK_SIZE) };

const short MeshBuilder::s_neighbouringBlocksX[6] = { 0, 0, -1, 1, 0, 0 };

const short MeshBuilder::s_neighbouringBlocksY[6] = { -1, 0, 0, 0, 0, 1 };

const short MeshBuilder::s_neighbouringBlocksZ[6] = { 0, -1, 0, 0, 1, 0 };

const short MeshBuilder::s_adjacentBlocksToFaceOffests[48] = { -1 - constants::CHUNK_SIZE, -constants::CHUNK_SIZE, -constants::CHUNK_SIZE + 1, 1, 1 + constants::CHUNK_SIZE, constants::CHUNK_SIZE, constants::CHUNK_SIZE - 1, -1,
                                                          1 - (constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE) - 1, -1, -1 + constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1, 1,
                                                          -constants::CHUNK_SIZE - (constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE) + constants::CHUNK_SIZE, constants::CHUNK_SIZE, constants::CHUNK_SIZE + constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE - constants::CHUNK_SIZE, -constants::CHUNK_SIZE,
                                                          constants::CHUNK_SIZE - (constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE) - constants::CHUNK_SIZE, -constants::CHUNK_SIZE, -constants::CHUNK_SIZE + constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE + constants::CHUNK_SIZE, constants::CHUNK_SIZE,
                                                          -1 - (constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -(constants::CHUNK_SIZE * constants::CHUNK_SIZE) + 1, 1, 1 + constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE, constants::CHUNK_SIZE * constants::CHUNK_SIZE - 1, -1,
                                                          -1 + constants::CHUNK_SIZE, constants::CHUNK_SIZE, constants::CHUNK_SIZE + 1, 1, 1 - constants::CHUNK_SIZE, -constants::CHUNK_SIZE, -constants::CHUNK_SIZE - 1, -1 };

const short MeshBuilder::s_adjacentBlocksToFaceOffestsX[48] = { -1, 0, 1, 1, 1, 0, -1, -1,
                                                          1, 0, -1, -1, -1, 0, 1, 1,
                                                          0, 0, 0, 0, 0, 0, 0, 0,
                                                          0, 0, 0, 0, 0, 0, 0, 0,
                                                          -1, 0, 1, 1, 1, 0, -1, -1,
                                                          -1, 0, 1, 1, 1, 0, -1, -1 };

const short MeshBuilder::s_adjacentBlocksToFaceOffestsY[48] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                                          -1, -1, -1, 0, 1, 1, 1, 0,
                                                          -1, -1, -1, 0, 1, 1, 1, 0,
                                                          -1, -1, -1, 0, 1, 1, 1, 0,
                                                          -1, -1, -1, 0, 1, 1, 1, 0,
                                                          0, 0, 0, 0, 0, 0, 0, 0 };

const short MeshBuilder::s_adjacentBlocksToFaceOffestsZ[48] = { -1, -1, -1, 0, 1, 1, 1, 0,
                                                          0, 0, 0, 0, 0, 0, 0, 0,
                                                          -1, 0, 1, 1, 1, 0, -1, -1,
                                                          1, 0, -1, -1, -1, 0, 1, 1,
                                                          0, 0, 0, 0, 0, 0, 0, 0,
                                                          1, 1, 1, 0, -1, -1, -1, 0 };

void MeshBuilder::addFaceToMesh(float* vertices, unsigned int* numVertices, unsigned int* indices, unsigned int* numIndices, float* waterVertices, unsigned int* numWaterVertices, unsigned int* waterIndices, unsigned int* numWaterIndices, unsigned int block, short neighbouringBlock) {
    int chunkPosition[3];
    m_chunk.getChunkPosition(chunkPosition);
    int blockCoords[3];
    int neighbouringBlockPos[3];
    findBlockCoordsInChunk(blockCoords, block);
    neighbouringBlockPos[0] = chunkPosition[0] * constants::CHUNK_SIZE + blockCoords[0] + s_neighbouringBlocksX[neighbouringBlock];
    neighbouringBlockPos[1] = chunkPosition[1] * constants::CHUNK_SIZE + blockCoords[1] + s_neighbouringBlocksY[neighbouringBlock];
    neighbouringBlockPos[2] = chunkPosition[2] * constants::CHUNK_SIZE + blockCoords[2] + s_neighbouringBlocksZ[neighbouringBlock];

    if (constants::cubeMesh[m_chunk.getBlock(block)]) {
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

        if (m_chunk.getBlock(block) == 4) {
            //TODO
            //find whether there is a block above the water to see if the water should be rendered as a full block or a surface block
            bool waterAbove;
            if (block >= (constants::CHUNK_SIZE * constants::CHUNK_SIZE * (constants::CHUNK_SIZE - 1))) {
                int blockCoords[3];
                //wip
            }

            float texCoords[8];
            getTextureCoordinates(texCoords, s_blockIdToTextureNum[m_chunk.getBlock(block) * 6 + textureNum]);
            for (short vertex = 0; vertex < 4; vertex++) {
                for (short element = 0; element < 3; element++) {
                    waterVertices[*numWaterVertices] = constants::CUBE_FACE_POSITIONS[firstPositionIndex + vertex * 3 + element] + blockCoords[element];
                    (*numWaterVertices)++;
                }
                waterVertices[*numWaterVertices] = texCoords[vertex * 2];
                (*numWaterVertices)++;
                waterVertices[*numWaterVertices] = texCoords[vertex * 2 + 1];
                (*numWaterVertices)++;
                waterVertices[*numWaterVertices] = (1.0f / 16.0f) * 16;//(m_chunk.getWorldSkyLight(blockCoords) + 1);
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
                blockCoords[1]++;

                getTextureCoordinates(texCoords, s_blockIdToTextureNum[m_chunk.getBlock(block) * 6 + textureNum]);
                for (short vertex = 0; vertex < 4; vertex++) {
                    for (short element = 0; element < 3; element++) {
                        waterVertices[*numWaterVertices] = constants::CUBE_FACE_POSITIONS[firstPositionIndex + vertex * 3 + element] + blockCoords[element];
                        (*numWaterVertices)++;
                    }
                    waterVertices[*numWaterVertices] = texCoords[vertex * 2];
                    (*numWaterVertices)++;
                    waterVertices[*numWaterVertices] = texCoords[vertex * 2 + 1];
                    (*numWaterVertices)++;
                    waterVertices[*numWaterVertices] = (1.0f / 16.0f) * (m_chunk.getWorldSkyLight(neighbouringBlockPos) + 1);
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
            getTextureCoordinates(texCoords, s_blockIdToTextureNum[m_chunk.getBlock(block) * 6 + textureNum]);
            for (short vertex = 0; vertex < 4; vertex++) {
                for (short element = 0; element < 3; element++) {
                    vertices[*numVertices] = constants::CUBE_FACE_POSITIONS[firstPositionIndex + vertex * 3 + element] + blockCoords[element];
                    (*numVertices)++;
                }
                vertices[*numVertices] = texCoords[vertex * 2];
                (*numVertices)++;
                vertices[*numVertices] = texCoords[vertex * 2 + 1];
                (*numVertices)++;
                vertices[*numVertices] = (1.0f / 16.0f) * 16;//(m_chunk.getWorldSkyLight(neighbouringBlockPos) + 1);
                (*numVertices)++;
            }

            //ambient occlusion
            short blockType;
            for (unsigned char adjacentBlockToFace = 0; adjacentBlockToFace < 8; adjacentBlockToFace++) {
                int blockCoords[3];
                blockCoords[0] = block % constants::CHUNK_SIZE;
                blockCoords[1] = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
                blockCoords[2] = (block - blockCoords[1] * constants::CHUNK_SIZE * constants::CHUNK_SIZE) / constants::CHUNK_SIZE;
                blockCoords[0] += chunkPosition[0] * constants::CHUNK_SIZE + s_neighbouringBlocksX[neighbouringBlockCoordsOffset] + s_adjacentBlocksToFaceOffestsX[firstAdjacentBlockIndex + adjacentBlockToFace];
                blockCoords[1] += chunkPosition[1] * constants::CHUNK_SIZE + s_neighbouringBlocksY[neighbouringBlockCoordsOffset] + s_adjacentBlocksToFaceOffestsY[firstAdjacentBlockIndex + adjacentBlockToFace];
                blockCoords[2] += chunkPosition[2] * constants::CHUNK_SIZE + s_neighbouringBlocksZ[neighbouringBlockCoordsOffset] + s_adjacentBlocksToFaceOffestsZ[firstAdjacentBlockIndex + adjacentBlockToFace];
                blockType = m_chunk.getWorldBlock(blockCoords);
                if (constants::castsShadows[blockType]) {
                    if (*numVertices < 24) {
                        std::cout << "num vertices: " << *numVertices << "\n";
                    }
                    unsigned char type = m_chunk.getBlock(block);
                    float val = constants::shadowReceiveAmount[type];
                    int vertexNum = *numVertices - (19 - adjacentBlockToFace / 2 * 6);
                    vertices[vertexNum] *= val;
                    //vertices[*numVertices - (19 - adjacentBlockToFace / 2 * 6)] *= constants::shadowReceiveAmount[m_chunk.getBlock(block)];
                    if ((adjacentBlockToFace % 2) != 0) {
                        vertexNum = *numVertices - (19 - ((adjacentBlockToFace / 2 + 1) % 4) * 6);
                        vertices[vertexNum] *= val;
                        //vertices[*numVertices - (19 - ((adjacentBlockToFace / 2 + 1) % 4) * 6)] *= constants::shadowReceiveAmount[m_chunk.getBlock(block)];
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
            getTextureCoordinates(texCoords, s_blockIdToTextureNum[m_chunk.getBlock(block) * 6 + face]);
            for (short vertex = 0; vertex < 4; vertex++) {
                for (short element = 0; element < 3; element++) {
                    vertices[*numVertices] = constants::X_FACE_POSITIONS[firstPositionIndex + vertex * 3 + element] + blockCoords[element];
                    (*numVertices)++;
                }
                vertices[*numVertices] = texCoords[vertex * 2];
                (*numVertices)++;
                vertices[*numVertices] = texCoords[vertex * 2 + 1];
                (*numVertices)++;
                vertices[*numVertices] = (1.0f / 16.0f) * (m_chunk.getWorldSkyLight(neighbouringBlockPos) + 1);
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

void MeshBuilder::getTextureCoordinates(float* coords, short textureNum) {
    coords[0] = textureNum % 227 * 0.00439453125f + 0.000244140625f;
    coords[1] = 0.9956054688f - (textureNum / 227 * 0.00439453125f) + 0.000244140625f;
    coords[2] = coords[0] + 0.00390625f;
    coords[3] = coords[1];
    coords[4] = coords[2];
    coords[5] = coords[1] + 0.00390625f;
    coords[6] = coords[0];
    coords[7] = coords[5];
}

void MeshBuilder::buildMesh(float* vertices, unsigned int* numVertices, unsigned int* indices, unsigned int* numIndices, float* waterVertices, unsigned int* numWaterVertices, unsigned int* waterIndices, unsigned int* numWaterIndices) {
    // if (!m_chunk.isSkylightUpToDate()) {
    //     bool neighbouringChunksToRelight[6];
    //     s_checkingNeighbouringRelights.lock();
    //     bool neighbourBeingRelit = true;
    //     while (neighbourBeingRelit) {
    //         neighbourBeingRelit = false;
    //         for (unsigned int i = 0; i < 6; i++) {
    //             neighbourBeingRelit |= m_worldInfo.worldChunks[neighbouringChunkIndices[i]].skyBeingRelit();
    //         }
    //         std::this_thread::sleep_for(std::chrono::microseconds(100));
    //     }
    //     m_calculatingSkylight = true;
    //     clearSkyLight();
    //     calculateSkyLight(neighbouringChunkIndices, neighbouringChunksToRelight);
    //     m_calculatingSkylight = false;
    // }
    *numVertices = *numIndices = *numWaterVertices = *numWaterIndices = 0;
    int chunkPosition[3];
    m_chunk.getChunkPosition(chunkPosition);
    int blockPos[3];
    int neighbouringBlockPos[3];
    int blockNum = 0;
    for (blockPos[1] = chunkPosition[1] * constants::CHUNK_SIZE; blockPos[1] < (chunkPosition[1] + 1) * constants::CHUNK_SIZE; blockPos[1]++) {
        for (blockPos[2] = chunkPosition[2] * constants::CHUNK_SIZE; blockPos[2] < (chunkPosition[2] + 1) * constants::CHUNK_SIZE; blockPos[2]++) {
            for (blockPos[0] = chunkPosition[0] * constants::CHUNK_SIZE; blockPos[0] < (chunkPosition[0] + 1) * constants::CHUNK_SIZE; blockPos[0]++) {
                if (m_chunk.getBlock(blockNum) == 0) {
                    blockNum++;
                    continue;
                }
                if (m_chunk.getBlock(blockNum) == 4) {
                    for (short neighbouringBlock = 0; neighbouringBlock < 6; neighbouringBlock++) {
                        neighbouringBlockPos[0] = blockPos[0] + s_neighbouringBlocksX[neighbouringBlock];
                        neighbouringBlockPos[1] = blockPos[1] + s_neighbouringBlocksY[neighbouringBlock];
                        neighbouringBlockPos[2] = blockPos[2] + s_neighbouringBlocksZ[neighbouringBlock];
                        unsigned char neighbouringBlockType =m_chunk.getWorldBlock(neighbouringBlockPos);
                        if ((neighbouringBlockType != 4) && (constants::transparent[neighbouringBlockType])) {
                            addFaceToMesh(vertices, numVertices, indices, numIndices, waterVertices, numWaterVertices, waterIndices, numWaterIndices, blockNum, neighbouringBlock);
                        }
                    }
                }
                else {
                    for (short neighbouringBlock = 0; neighbouringBlock < 6; neighbouringBlock++) {
                        neighbouringBlockPos[0] = blockPos[0] + s_neighbouringBlocksX[neighbouringBlock];
                        neighbouringBlockPos[1] = blockPos[1] + s_neighbouringBlocksY[neighbouringBlock];
                        neighbouringBlockPos[2] = blockPos[2] + s_neighbouringBlocksZ[neighbouringBlock];
                        if (constants::transparent[m_chunk.getWorldBlock(neighbouringBlockPos)]) {
                            addFaceToMesh(vertices, numVertices, indices, numIndices, waterVertices, numWaterVertices, waterIndices, numWaterIndices, blockNum, neighbouringBlock);
                        }
                    }
                }
                blockNum++;
            }
        }
    }
}

}  // namespace client