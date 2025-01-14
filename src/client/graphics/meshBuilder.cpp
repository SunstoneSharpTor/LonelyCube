/*
  Lonely Cube, a voxel game
  Copyright (C) g 2024-2025 Bertie Cartwright

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

namespace lonelycube {

namespace client{

const int16_t MeshBuilder::s_neighbouringBlocksX[7] = { 0, 0, -1, 1, 0, 0,  0 };

const int16_t MeshBuilder::s_neighbouringBlocksY[7] = { -1, 0, 0, 0, 0, 1,  0 };

const int16_t MeshBuilder::s_neighbouringBlocksZ[7] = { 0, -1, 0, 0, 1, 0,  0 };

MeshBuilder::MeshBuilder(Chunk& chunk, ServerWorld<true>& serverWorld, float* vertices, uint32_t* numVertices, uint32_t* indices, uint32_t* numIndices, float* waterVertices,
    uint32_t* numWaterVertices, uint32_t* waterIndices, uint32_t* numWaterIndices)
    : m_chunk(chunk), m_serverWorld(serverWorld), m_vertices(vertices),
    m_numVertices(numVertices), m_indices(indices), m_numIndices(numIndices),
    m_waterVertices(waterVertices), m_numWaterVertices(numWaterVertices),
    m_waterIndices(waterIndices), m_numWaterIndices(numWaterIndices)
{
    m_chunk.getPosition(m_chunkPosition);
    m_chunkWorldCoords[0] = m_chunkPosition[0] * constants::CHUNK_SIZE;
    m_chunkWorldCoords[1] = m_chunkPosition[1] * constants::CHUNK_SIZE;
    m_chunkWorldCoords[2] = m_chunkPosition[2] * constants::CHUNK_SIZE;
}

void MeshBuilder::addFaceToMesh(uint32_t block, uint8_t blockType, uint8_t faceNum)
{
    BlockData blockData = m_serverWorld.getResourcePack().getBlockData(blockType);
    Face faceData = blockData.model->faces[faceNum];
    int blockCoords[3];
    int lightingBlockPos[3];
    int worldBlockPos[3];
    findBlockCoordsInChunk(blockCoords, block);
    lightingBlockPos[0] = m_chunkWorldCoords[0] + blockCoords[0] + s_neighbouringBlocksX[faceData.lightingBlock];
    lightingBlockPos[1] = m_chunkWorldCoords[1] + blockCoords[1] + s_neighbouringBlocksY[faceData.lightingBlock];
    lightingBlockPos[2] = m_chunkWorldCoords[2] + blockCoords[2] + s_neighbouringBlocksZ[faceData.lightingBlock];
    worldBlockPos[0] = m_chunkWorldCoords[0] + blockCoords[0];
    worldBlockPos[1] = m_chunkWorldCoords[1] + blockCoords[1];
    worldBlockPos[2] = m_chunkWorldCoords[2] + blockCoords[2];
    if (blockType == 4)
    {
        float texCoords[8];
        ResourcePack::getTextureCoordinates(
            texCoords, faceData.UVcoords, blockData.faceTextureIndices[faceNum]);
        for (int16_t vertex = 0; vertex < 4; vertex++)
        {
            for (int16_t element = 0; element < 3; element++)
            {
                m_waterVertices[*m_numWaterVertices] = faceData.coords[vertex * 3 + element] +
                    blockCoords[element] + 0.5f;
                (*m_numWaterVertices)++;
            }
            m_waterVertices[*m_numWaterVertices] = texCoords[vertex * 2];
            m_waterVertices[*m_numWaterVertices + 1] = texCoords[vertex * 2 + 1];
            m_waterVertices[*m_numWaterVertices + 2] = getSmoothSkyLight(worldBlockPos,
                faceData.coords + vertex * 3, faceData.lightingBlock);
            m_waterVertices[*m_numWaterVertices + 3] = getSmoothBlockLight(worldBlockPos,
                faceData.coords + vertex * 3, faceData.lightingBlock);
            (*m_numWaterVertices) += 4;
        }

        //index buffer
        int trueNumVertices = *m_numWaterVertices / 7;
        m_waterIndices[*m_numWaterIndices] = trueNumVertices - 4;
        m_waterIndices[*m_numWaterIndices + 1] = trueNumVertices - 3;
        m_waterIndices[*m_numWaterIndices + 2] = trueNumVertices - 2;
        m_waterIndices[*m_numWaterIndices + 4] = trueNumVertices - 2;
        m_waterIndices[*m_numWaterIndices + 5] = trueNumVertices - 1;
        m_waterIndices[*m_numWaterIndices + 3] = trueNumVertices - 4;
        *m_numWaterIndices += 6;
    }
    else
    {
        float texCoords[8];
        ResourcePack::getTextureCoordinates(
            texCoords, faceData.UVcoords, blockData.faceTextureIndices[faceNum]
        );
        for (int16_t vertex = 0; vertex < 4; vertex++)
        {
            for (int16_t element = 0; element < 3; element++)
            {
                m_vertices[*m_numVertices] = faceData.coords[vertex * 3 + element] +
                    blockCoords[element] + 0.5f;
                (*m_numVertices)++;
            }
            m_vertices[*m_numVertices] = texCoords[vertex * 2];
            m_vertices[*m_numVertices + 1] = texCoords[vertex * 2 + 1];
            float ambientOcclusion = getAmbientOcclusion(worldBlockPos, faceData.coords + vertex *
                3, faceData.lightingBlock);
            m_vertices[*m_numVertices + 2] = getSmoothSkyLight(worldBlockPos, faceData.coords +
                vertex * 3, faceData.lightingBlock) * ambientOcclusion;
            m_vertices[*m_numVertices + 3] = getSmoothBlockLight(worldBlockPos, faceData.coords +
                vertex * 3, faceData.lightingBlock) * ambientOcclusion;
            (*m_numVertices) += 4;
        }

        //index buffer
        int trueNumVertices = *m_numVertices / 7;
        m_indices[*m_numIndices] = trueNumVertices - 4;
        m_indices[*m_numIndices + 1] = trueNumVertices - 3;
        m_indices[*m_numIndices + 2] = trueNumVertices - 2;
        m_indices[*m_numIndices + 4] = trueNumVertices - 2;
        m_indices[*m_numIndices + 5] = trueNumVertices - 1;
        m_indices[*m_numIndices + 3] = trueNumVertices - 4;
        (*m_numIndices) += 6;
    }
}

float MeshBuilder::getSmoothSkyLight(int* blockCoords, float* pointCoords, int8_t direction)
{
    if (direction == 6)
    {
        return (float)m_serverWorld.chunkManager.getSkyLight(blockCoords) / constants::skyLightMaxValue;
    }
    else
    {
        const int mask1[4] = { 0, 1, 0, 1 };
        const int mask2[4] = { 0, 0, 1, 1 };
        const int edges[4] = { 0, 1, 1, 0 };

        int cornerOffset[3];
        for (int8_t i = 0; i < 3; i++)
        {
            cornerOffset[i] = ((pointCoords[i] - 0.5f) > 0) * 2 - 1;
        }

        int fixed = (s_neighbouringBlocksY[direction] != 0) + 2 * (s_neighbouringBlocksZ[direction]
            != 0);
        int unfixed1 = fixed == 0;
        int unfixed2 = 2 - (fixed == 2);

        int testBlockCoords[3];
        testBlockCoords[fixed] = blockCoords[fixed] + (direction > 2) * 2 - 1;
        float brightness = 0;
        int transparrentBlocks = 0;
        int numEdgesBlocked = 0;
        for (int8_t i = 0; i < 4 && numEdgesBlocked < 2; i++)
        {
            testBlockCoords[unfixed1] = blockCoords[unfixed1] + mask1[i] * cornerOffset[unfixed1];
            testBlockCoords[unfixed2] = blockCoords[unfixed2] + mask2[i] * cornerOffset[unfixed2];
            bool transparrentBlock = m_serverWorld.getResourcePack().getBlockData(
                m_serverWorld.chunkManager.getBlock(testBlockCoords)).transparent;
            brightness += m_serverWorld.chunkManager.getSkyLight(testBlockCoords) * transparrentBlock;
            transparrentBlocks += transparrentBlock;
            numEdgesBlocked += !transparrentBlock * edges[i];
        }
        return brightness / transparrentBlocks / constants::skyLightMaxValue;
    }
}

float MeshBuilder::getSmoothBlockLight(int* blockCoords, float* pointCoords, int8_t direction)
{
    if (direction == 6)
    {
        return (float)m_serverWorld.chunkManager.getBlockLight(blockCoords) / constants::blockLightMaxValue;
    }
    else
    {
        const int mask1[4] = { 0, 1, 0, 1 };
        const int mask2[4] = { 0, 0, 1, 1 };
        const int edges[4] = { 0, 1, 1, 0 };

        int cornerOffset[3];
        for (int8_t i = 0; i < 3; i++)
        {
            cornerOffset[i] = ((pointCoords[i] - 0.5f) > 0) * 2 - 1;
        }

        int fixed = (s_neighbouringBlocksY[direction] != 0) + 2 * (s_neighbouringBlocksZ[direction]
            != 0);
        int unfixed1 = fixed == 0;
        int unfixed2 = 2 - (fixed == 2);

        int testBlockCoords[3];
        testBlockCoords[fixed] = blockCoords[fixed] + (direction > 2) * 2 - 1;
        float brightness = 0;
        int transparrentBlocks = 0;
        int numEdgesBlocked = 0;
        for (int8_t i = 0; i < 4 && numEdgesBlocked < 2; i++)
        {
            testBlockCoords[unfixed1] = blockCoords[unfixed1] + mask1[i] * cornerOffset[unfixed1];
            testBlockCoords[unfixed2] = blockCoords[unfixed2] + mask2[i] * cornerOffset[unfixed2];
            bool transparrentBlock = m_serverWorld.getResourcePack().getBlockData(
                m_serverWorld.chunkManager.getBlock(testBlockCoords)).transparent;
            brightness += m_serverWorld.chunkManager.getBlockLight(testBlockCoords) * transparrentBlock;
            transparrentBlocks += transparrentBlock;
            numEdgesBlocked += !transparrentBlock * edges[i];
        }
        return brightness / transparrentBlocks / constants::blockLightMaxValue;
    }
}

float MeshBuilder::getAmbientOcclusion(int* blockCoords, float* pointCoords, int8_t direction)
{
    if (direction == 6)
    {
        return 1.0f;
    }
    else
    {
        const int mask1[3] = { 1, 0, 1 };
        const int mask2[3] = { 0, 1, 1 };
        const int corners[3] = { 0, 0, 1 };

        int cornerOffset[3];
        for (int8_t i = 0; i < 3; i++)
        {
            cornerOffset[i] = ((pointCoords[i] - 0.5f) > 0) * 2 - 1;
        }

        int fixed = (s_neighbouringBlocksY[direction] != 0) + 2 * (s_neighbouringBlocksZ[direction] != 0);
        int unfixed1 = fixed == 0;
        int unfixed2 = 2 - (fixed == 2);

        int testBlockCoords[3];
        testBlockCoords[fixed] = blockCoords[fixed] + (direction > 2) * 2 - 1;
        int numSideOccluders = 0;
        int numCornerOccluders = 0;
        for (int8_t i = 0; i < 3; i++)
        {
            testBlockCoords[unfixed1] = blockCoords[unfixed1] + mask1[i] * cornerOffset[unfixed1];
            testBlockCoords[unfixed2] = blockCoords[unfixed2] + mask2[i] * cornerOffset[unfixed2];
            bool occluder = m_serverWorld.getResourcePack().getBlockData(
                m_serverWorld.chunkManager.getBlock(testBlockCoords)
            ).castsAmbientOcclusion;
            bool corner = corners[i];
            numSideOccluders += occluder * (1 - corner);
            numCornerOccluders += occluder * (corner);
        }
        numCornerOccluders |= numSideOccluders == 2;
        // Square root the value as it is going to be squared by the shader
        return std::sqrt((7.0f - numSideOccluders - numCornerOccluders)) / 2.6458f;  // sqrt(7) = 2.6458
    }
}

void MeshBuilder::buildMesh()
{
    *m_numVertices = *m_numIndices = *m_numWaterVertices = *m_numWaterIndices = 0;
    int chunkPosition[3];
    m_chunk.getPosition(chunkPosition);
    int blockPos[3];
    int neighbouringBlockPos[3];
    int blockNum = 0;
    int layerNum = 0;
    for (blockPos[1] = chunkPosition[1] * constants::CHUNK_SIZE; blockPos[1] < (chunkPosition[1] + 1) * constants::CHUNK_SIZE; blockPos[1]++)
    {
        uint16_t layerBlockType = m_chunk.getLayerBlockType(layerNum);
        if (layerBlockType < 256 && layerNum > 0 && layerNum < constants::CHUNK_SIZE - 1 &&
            m_chunk.getLayerBlockType(layerNum - 1) == layerBlockType &&
            m_chunk.getLayerBlockType(layerNum + 1) == layerBlockType)
        {
            blockNum += constants::CHUNK_SIZE * constants::CHUNK_SIZE;
            layerNum++;
            continue;
        }
        for (blockPos[2] = chunkPosition[2] * constants::CHUNK_SIZE; blockPos[2] < (chunkPosition[2] + 1) * constants::CHUNK_SIZE; blockPos[2]++)
        {
            for (blockPos[0] = chunkPosition[0] * constants::CHUNK_SIZE; blockPos[0] < (chunkPosition[0] + 1) * constants::CHUNK_SIZE; blockPos[0]++)
            {
                uint8_t blockType = m_chunk.getBlock(blockNum);
                if (blockType == 0)
                {
                    blockNum++;
                    continue;
                }
                if (blockType == 4)
                {
                    for (uint8_t faceNum = 0 ; faceNum < m_serverWorld.getResourcePack().
                        getBlockData(blockType).model->numFaces; faceNum++)
                    {
                        int8_t cullFace = m_serverWorld.getResourcePack().getBlockData(blockType).
                            model->faces[faceNum].cullFace;
                        neighbouringBlockPos[0] = blockPos[0] + s_neighbouringBlocksX[cullFace];
                        neighbouringBlockPos[1] = blockPos[1] + s_neighbouringBlocksY[cullFace];
                        neighbouringBlockPos[2] = blockPos[2] + s_neighbouringBlocksZ[cullFace];
                        uint8_t neighbouringBlockType = m_serverWorld.chunkManager.getBlock(neighbouringBlockPos);
                        if ((neighbouringBlockType != 4) && (m_serverWorld.getResourcePack().
                            getBlockData(neighbouringBlockType).transparent))
                        {
                            addFaceToMesh(blockNum, blockType, faceNum);
                        }
                    }
                }
                else
                {
                    for (uint8_t faceNum = 0 ; faceNum < m_serverWorld.getResourcePack().
                        getBlockData(blockType).model->numFaces; faceNum++)
                    {
                        int8_t cullFace = m_serverWorld.getResourcePack().getBlockData(blockType).
                            model->faces[faceNum].cullFace;
                        neighbouringBlockPos[0] = blockPos[0] + s_neighbouringBlocksX[cullFace];
                        neighbouringBlockPos[1] = blockPos[1] + s_neighbouringBlocksY[cullFace];
                        neighbouringBlockPos[2] = blockPos[2] + s_neighbouringBlocksZ[cullFace];
                        // TODO: investigate splitting up the line below into multiple lines of code (causes bugs)
                        if (cullFace < 0 || m_serverWorld.getResourcePack().getBlockData(
                            m_serverWorld.chunkManager.getBlock(neighbouringBlockPos)).transparent)
                        {
                            addFaceToMesh(blockNum, blockType, faceNum);
                        }
                    }
                }
                blockNum++;
            }
        }
        layerNum++;
    }
}

}  // namespace client

}  // namespace lonelycube
