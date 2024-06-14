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

#include <iostream>
#include <cmath>
#include <queue>
#include <thread>

#include "glm/glm.hpp"
#include "glm/gtc/noise.hpp"

#include "core/position.h"
#include "core/random.h"

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

const short Chunk::neighbouringBlocks[6] = { -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -constants::CHUNK_SIZE, -1, 1, constants::CHUNK_SIZE, (constants::CHUNK_SIZE * constants::CHUNK_SIZE) };

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

Chunk::Chunk(Position position) {
    inUse = true;
    m_singleBlockType = false;
    m_singleSkyLightVal = false;
    m_skyLightUpToDate = false;
    m_calculatingSkylight = false;
    m_playerCount = 0;

    m_blocks = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
    m_skyLight = new unsigned char[(constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];

    m_position[0] = position.x;
    m_position[1] = position.y;
    m_position[2] = position.z;
}

Chunk::Chunk(int x, int y, int z, WorldInfo wio) {
    inUse = true;
    m_singleBlockType = false;
    m_singleSkyLightVal = false;
    m_skyLightUpToDate = false;
    m_calculatingSkylight = false;
    m_playerCount = 0;
    m_worldInfo = wio;

    m_blocks = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
    m_skyLight = new unsigned char[(constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];

    m_position[0] = x;
    m_position[1] = y;
    m_position[2] = z;
}

Chunk::Chunk(WorldInfo wio) {
    inUse = false;
    m_singleBlockType = false;
    m_singleSkyLightVal = false;
    m_skyLightUpToDate = false;
    m_calculatingSkylight = false;
    m_playerCount = 0;
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
    m_playerCount = 0;
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
    m_playerCount = 0;
    delete[] m_blocks;
    delete[] m_skyLight;
    m_blocks = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
    m_skyLight = new unsigned char[(constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];

    m_position[0] = x;
    m_position[1] = y;
    m_position[2] = z;
}

void Chunk::setWorldInfo(WorldInfo wio) {
    m_worldInfo = wio;
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

    //TODO:    make it so that chunks of a single block type get meshes without calling the new function
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

    //TODO:    make it so that chunks of a single block type get meshes without calling the new function
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

void Chunk::setBlock(unsigned int block, unsigned char blockType) {
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

void Chunk::clearBlocksAndLight() {
    //reset all blocks in the chunk to air and all skylight values to 0
    for (unsigned int blockNum = 0; blockNum < constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE; blockNum++) {
        m_blocks[blockNum] = 0;
        m_skyLight[blockNum >> 1] = 0;
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

    //(*m_worldInfo.numRelights)++;

    //TODO:    make it so that chunks of a single block type get meshes without calling the new function
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
            unsigned int neighbouringBlockNum = blockNum + neighbouringBlocks[5];
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
            unsigned int neighbouringBlockNum = blockNum + neighbouringBlocks[4];
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
            unsigned int neighbouringBlockNum = blockNum + neighbouringBlocks[3];
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
            unsigned int neighbouringBlockNum = blockNum + neighbouringBlocks[2];
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
            unsigned int neighbouringBlockNum = blockNum + neighbouringBlocks[1];
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
            unsigned int neighbouringBlockNum = blockNum + neighbouringBlocks[0];
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

    //TODO:    make it so that chunks of a single block type get meshes without calling the new function
    if (m_singleBlockType) {
        unsigned char* temp = m_blocks;
        m_blocks = tempBlocks;
        delete[] temp;
    }

    
    //(*m_worldInfo.numRelights)--;
    m_skyLightUpToDate = true;
    m_calculatingSkylight = false;
    // lock release
    // std::lock_guard<std::mutex> lock(m_accessingSkylightMtx);
    // m_accessingSkylightCV.notify_all();
}

// void Chunk::uncompressBlocks() {
//     if (m_singleBlockType) {
//         delete m_blocks;
//         m_blocks = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
//         for (unsigned int block = 0; block < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE); block++) {
//             m_blocks[block] = m_blocks[0];
//         }
//     }
// }

void Chunk::compressBlocks() {
    if (m_singleBlockType) {
        unsigned char blockType = m_blocks[0];
        delete[] m_blocks;
        m_blocks = new unsigned char[1];
        m_blocks[0] = blockType;
    }
}