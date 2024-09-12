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

#include "client/graphics/meshBuilder.h"

#include "core/pch.h"

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
                                                 5, 5, 5, 5, 6, 6, //oak log
                                                 7, 7, 7, 7, 7, 7, //oak leaves
                                                 8, 8, 8, 8, 8, 8 //tall grass
                                                 };

const short MeshBuilder::s_neighbouringBlocksX[7] = { 0, 0, -1, 1, 0, 0,  0 };

const short MeshBuilder::s_neighbouringBlocksY[7] = { -1, 0, 0, 0, 0, 1,  0 };

const short MeshBuilder::s_neighbouringBlocksZ[7] = { 0, -1, 0, 0, 1, 0,  0 };

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

MeshBuilder::MeshBuilder(Chunk& chunk, ResourcePack& resourcePack, float* vertices, unsigned int*
    numVertices, unsigned int* indices, unsigned int* numIndices, float* waterVertices,
    unsigned int* numWaterVertices, unsigned int* waterIndices, unsigned int* numWaterIndices)
    : m_chunk(chunk), m_resourcePack(resourcePack), m_vertices(vertices),
    m_numVertices(numVertices), m_indices(indices), m_numIndices(numIndices),
    m_waterVertices(waterVertices), m_numWaterVertices(numWaterVertices),
    m_waterIndices(waterIndices), m_numWaterIndices(numWaterIndices) {
    m_chunk.getChunkPosition(m_chunkPosition);
    m_chunkWorldCoords[0] = m_chunkPosition[0] * constants::CHUNK_SIZE;
    m_chunkWorldCoords[1] = m_chunkPosition[1] * constants::CHUNK_SIZE;
    m_chunkWorldCoords[2] = m_chunkPosition[2] * constants::CHUNK_SIZE;
}

void MeshBuilder::addFaceToMesh(unsigned int block, unsigned char blockType, unsigned char faceNum) {
    BlockData blockData = m_resourcePack.getBlockData(blockType);
    Face faceData = blockData.model->faces[faceNum];
    int blockCoords[3];
    int lightingBlockPos[3];
    findBlockCoordsInChunk(blockCoords, block);
    lightingBlockPos[0] = m_chunkWorldCoords[0] + blockCoords[0] + s_neighbouringBlocksX[faceData.lightingBlock];
    lightingBlockPos[1] = m_chunkWorldCoords[1] + blockCoords[1] + s_neighbouringBlocksY[faceData.lightingBlock];
    lightingBlockPos[2] = m_chunkWorldCoords[2] + blockCoords[2] + s_neighbouringBlocksZ[faceData.lightingBlock];
    if (blockType == 4) {
        float texCoords[8];
        getTextureCoordinates(texCoords, faceData.UVcoords, blockData.faceTextureIndices[faceNum]);
        for (short vertex = 0; vertex < 4; vertex++) {
            for (short element = 0; element < 3; element++) {
                m_waterVertices[*m_numWaterVertices] = faceData.coords[vertex * 3 + element] + blockCoords[element];
                (*m_numWaterVertices)++;
            }
            m_waterVertices[*m_numWaterVertices] = texCoords[vertex * 2];
            m_waterVertices[*m_numWaterVertices + 1] = texCoords[vertex * 2 + 1];
            m_waterVertices[*m_numWaterVertices + 2] = 1.0f;
            m_waterVertices[*m_numWaterVertices + 3] = m_chunk.getWorldSkyLight(lightingBlockPos);
            (*m_numWaterVertices) += 4;
        }

        //index buffer
        int trueNumVertices = *m_numWaterVertices / 7;
        m_waterIndices[*m_numWaterIndices] = trueNumVertices - 4;
        m_waterIndices[*m_numWaterIndices + 1] = trueNumVertices - 3;
        m_waterIndices[*m_numWaterIndices + 2] = trueNumVertices - 2;
        m_waterIndices[*m_numWaterIndices + 3] = trueNumVertices - 4;
        m_waterIndices[*m_numWaterIndices + 4] = trueNumVertices - 2;
        m_waterIndices[*m_numWaterIndices + 5] = trueNumVertices - 1;
        *m_numWaterIndices += 6;
    }
    else {
        float texCoords[8];
        getTextureCoordinates(texCoords, faceData.UVcoords, blockData.faceTextureIndices[faceNum]);
        for (short vertex = 0; vertex < 4; vertex++) {
            for (short element = 0; element < 3; element++) {
                m_vertices[*m_numVertices] = faceData.coords[vertex * 3 + element] + blockCoords[element];
                (*m_numVertices)++;
            }
            m_vertices[*m_numVertices] = texCoords[vertex * 2];
            m_vertices[*m_numVertices + 1] = texCoords[vertex * 2 + 1];
            m_vertices[*m_numVertices + 2] = 1.0f;
            m_vertices[*m_numVertices + 3] = m_chunk.getWorldSkyLight(lightingBlockPos);
            (*m_numVertices) += 4;
        }

        //ambient occlusion
        /*short blockType;
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
                unsigned char type = m_chunk.getBlock(block);
                float val = constants::shadowReceiveAmount[type];
                int vertexNum = *m_numVertices - (23 - adjacentBlockToFace / 2 * 7);
                m_vertices[vertexNum] *= val;
                if ((adjacentBlockToFace % 2) != 0) {
                    vertexNum = *m_numVertices - (23 - ((adjacentBlockToFace / 2 + 1) % 4) * 7);
                    m_vertices[vertexNum] *= val;
                }
            }
        }*/

        //index buffer
        int trueNumVertices = *m_numVertices / 7;
        m_indices[*m_numIndices] = trueNumVertices - 4;
        (*m_numIndices)++;
        m_indices[*m_numIndices] = trueNumVertices - 3;
        (*m_numIndices)++;
        m_indices[*m_numIndices] = trueNumVertices - 2;
        (*m_numIndices)++;
        m_indices[*m_numIndices] = trueNumVertices - 4;
        (*m_numIndices)++;
        m_indices[*m_numIndices] = trueNumVertices - 2;
        (*m_numIndices)++;
        m_indices[*m_numIndices] = trueNumVertices - 1;
        (*m_numIndices)++;
    }

    /*int chunkPosition[3];
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
            float texCoords[8];
            getTextureCoordinates(texCoords, s_blockIdToTextureNum[m_chunk.getBlock(block) * 6 + textureNum]);
            for (short vertex = 0; vertex < 4; vertex++) {
                for (short element = 0; element < 3; element++) {
                    m_waterVertices[*m_numWaterVertices] = constants::CUBE_FACE_POSITIONS[firstPositionIndex + vertex * 3 + element] + blockCoords[element];
                    (*m_numWaterVertices)++;
                }
                m_waterVertices[*m_numWaterVertices] = texCoords[vertex * 2];
                m_waterVertices[*m_numWaterVertices + 1] = texCoords[vertex * 2 + 1];
                m_waterVertices[*m_numWaterVertices + 2] = 1.0f;
                m_waterVertices[*m_numWaterVertices + 3] = m_chunk.getWorldSkyLight(neighbouringBlockPos);
                (*m_numWaterVertices) += 4;
            }

            //index buffer
            int trueNumVertices = *m_numWaterVertices / 7;
            m_waterIndices[*m_numWaterIndices] = trueNumVertices - 4;
            m_waterIndices[*m_numWaterIndices + 1] = trueNumVertices - 3;
            m_waterIndices[*m_numWaterIndices + 2] = trueNumVertices - 2;
            m_waterIndices[*m_numWaterIndices + 3] = trueNumVertices - 4;
            m_waterIndices[*m_numWaterIndices + 4] = trueNumVertices - 2;
            m_waterIndices[*m_numWaterIndices + 5] = trueNumVertices - 1;
            *m_numWaterIndices += 6;
        }
        else {
            float texCoords[8];
            getTextureCoordinates(texCoords, s_blockIdToTextureNum[m_chunk.getBlock(block) * 6 + textureNum]);
            for (short vertex = 0; vertex < 4; vertex++) {
                for (short element = 0; element < 3; element++) {
                    m_vertices[*m_numVertices] = constants::CUBE_FACE_POSITIONS[firstPositionIndex + vertex * 3 + element] + blockCoords[element];
                    (*m_numVertices)++;
                }
                m_vertices[*m_numVertices] = texCoords[vertex * 2];
                m_vertices[*m_numVertices + 1] = texCoords[vertex * 2 + 1];
                m_vertices[*m_numVertices + 2] = 1.0f;
                m_vertices[*m_numVertices + 3] = m_chunk.getWorldSkyLight(neighbouringBlockPos);
                (*m_numVertices) += 4;
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
                    unsigned char type = m_chunk.getBlock(block);
                    float val = constants::shadowReceiveAmount[type];
                    int vertexNum = *m_numVertices - (23 - adjacentBlockToFace / 2 * 7);
                    m_vertices[vertexNum] *= val;
                    if ((adjacentBlockToFace % 2) != 0) {
                        vertexNum = *m_numVertices - (23 - ((adjacentBlockToFace / 2 + 1) % 4) * 7);
                        m_vertices[vertexNum] *= val;
                    }
                }
            }

            //index buffer
            int trueNumVertices = *m_numVertices / 7;
           m_indices[*m_numIndices] = trueNumVertices - 4;
            (*m_numIndices)++;
           m_indices[*m_numIndices] = trueNumVertices - 3;
            (*m_numIndices)++;
           m_indices[*m_numIndices] = trueNumVertices - 2;
            (*m_numIndices)++;
           m_indices[*m_numIndices] = trueNumVertices - 4;
            (*m_numIndices)++;
           m_indices[*m_numIndices] = trueNumVertices - 2;
            (*m_numIndices)++;
           m_indices[*m_numIndices] = trueNumVertices - 1;
            (*m_numIndices)++;
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
                    m_vertices[*m_numVertices] = constants::X_FACE_POSITIONS[firstPositionIndex + vertex * 3 + element] + blockCoords[element];
                    (*m_numVertices)++;
                }
                m_vertices[*m_numVertices] = texCoords[vertex * 2];
                m_vertices[*m_numVertices + 1] = texCoords[vertex * 2 + 1];
                m_vertices[*m_numVertices + 2] = 1.0f;
                m_vertices[*m_numVertices + 3] = m_chunk.getWorldSkyLight(neighbouringBlockPos);
                (*m_numVertices) += 4;
            }

            //TODO
            //lighting for X-shaped meshes

            //index buffer
            int trueNumVertices = *m_numVertices / 7;
           m_indices[*m_numIndices] = trueNumVertices - 4;
            (*m_numIndices)++;
           m_indices[*m_numIndices] = trueNumVertices - 3;
            (*m_numIndices)++;
           m_indices[*m_numIndices] = trueNumVertices - 2;
            (*m_numIndices)++;
           m_indices[*m_numIndices] = trueNumVertices - 4;
            (*m_numIndices)++;
           m_indices[*m_numIndices] = trueNumVertices - 2;
            (*m_numIndices)++;
           m_indices[*m_numIndices] = trueNumVertices - 1;
            (*m_numIndices)++;
        }
    }*/
}

void MeshBuilder::getTextureCoordinates(float* coords, float* textureBox, const short textureNum) {
    coords[0] = 0.015625f + textureNum % 16 * 0.0625f + textureBox[0] * 0.03125;
    coords[1] = 0.953125 - textureNum / 16 * 0.0625f + textureBox[1] * 0.03125;
    coords[2] = coords[0] + 0.03125f - (textureBox[0] + 1.0f - textureBox[2]) * 0.03125;
    coords[3] = coords[1];
    coords[4] = coords[2];
    coords[5] = coords[1] + 0.03125f - (textureBox[1] + 1.0f - textureBox[3]) * 0.03125;
    coords[6] = coords[0];
    coords[7] = coords[5];
}

void MeshBuilder::buildMesh() {
    *m_numVertices = *m_numIndices = *m_numWaterVertices = *m_numWaterIndices = 0;
    int chunkPosition[3];
    m_chunk.getChunkPosition(chunkPosition);
    int blockPos[3];
    int neighbouringBlockPos[3];
    int blockNum = 0;
    for (blockPos[1] = chunkPosition[1] * constants::CHUNK_SIZE; blockPos[1] < (chunkPosition[1] + 1) * constants::CHUNK_SIZE; blockPos[1]++) {
        for (blockPos[2] = chunkPosition[2] * constants::CHUNK_SIZE; blockPos[2] < (chunkPosition[2] + 1) * constants::CHUNK_SIZE; blockPos[2]++) {
            for (blockPos[0] = chunkPosition[0] * constants::CHUNK_SIZE; blockPos[0] < (chunkPosition[0] + 1) * constants::CHUNK_SIZE; blockPos[0]++) {
                unsigned char blockType = m_chunk.getBlock(blockNum);
                if (blockType == 0) {
                    blockNum++;
                    continue;
                }
                if (blockType == 4) {
                    for (unsigned char faceNum = 0 ; faceNum < m_resourcePack.getBlockData(blockType).model->numFaces; faceNum++) {
                        char cullFace = m_resourcePack.getBlockData(blockType).model->faces[faceNum].cullFace;
                        neighbouringBlockPos[0] = blockPos[0] + s_neighbouringBlocksX[cullFace];
                        neighbouringBlockPos[1] = blockPos[1] + s_neighbouringBlocksY[cullFace];
                        neighbouringBlockPos[2] = blockPos[2] + s_neighbouringBlocksZ[cullFace];
                        unsigned char neighbouringBlockType = m_chunk.getWorldBlock(neighbouringBlockPos);
                        if ((neighbouringBlockType != 4) && (constants::transparent[neighbouringBlockType])) {
                            addFaceToMesh(blockNum, blockType, faceNum);
                        }
                    }
                }
                else {
                    for (unsigned char faceNum = 0 ; faceNum < m_resourcePack.getBlockData(blockType).model->numFaces; faceNum++) {
                        char cullFace = m_resourcePack.getBlockData(blockType).model->faces[faceNum].cullFace;
                        neighbouringBlockPos[0] = blockPos[0] + s_neighbouringBlocksX[cullFace];
                        neighbouringBlockPos[1] = blockPos[1] + s_neighbouringBlocksY[cullFace];
                        neighbouringBlockPos[2] = blockPos[2] + s_neighbouringBlocksZ[cullFace];
                        if (cullFace < 0 || constants::transparent[m_chunk.getWorldBlock(neighbouringBlockPos)]) {
                            addFaceToMesh(blockNum, blockType, faceNum);
                        }
                    }
                }
                blockNum++;
            }
        }
    }
}

}  // namespace client