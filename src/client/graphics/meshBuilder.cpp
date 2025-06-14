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

#include "client/graphics/meshBuilder.h"

#include "core/pch.h"

#include "core/chunk.h"
#include "core/constants.h"
#include <cmath>
#include <iomanip>

namespace lonelycube::client {

const int MeshBuilder::s_neighbouringBlocksX[7] = { 0, 0, -1, 1, 0, 0,  0 };

const int MeshBuilder::s_neighbouringBlocksY[7] = { -1, 0, 0, 0, 0, 1,  0 };

const int MeshBuilder::s_neighbouringBlocksZ[7] = { 0, -1, 0, 0, 1, 0,  0 };

MeshBuilder::MeshBuilder(
    Chunk& chunk, ServerWorld<true>& serverWorld, std::vector<float>& vertices,
    std::vector<uint32_t>& indices, std::vector<float>& waterVertices,
    std::vector<uint32_t>& waterIndices
) : m_chunk(chunk), m_serverWorld(serverWorld), m_vertices(vertices), m_indices(indices),
    m_waterVertices(waterVertices), m_waterIndices(waterIndices)
{
    m_chunk.getPosition(m_chunkPosition);
    m_chunkWorldCoords[0] = m_chunkPosition[0] * constants::CHUNK_SIZE;
    m_chunkWorldCoords[1] = m_chunkPosition[1] * constants::CHUNK_SIZE;
    m_chunkWorldCoords[2] = m_chunkPosition[2] * constants::CHUNK_SIZE;
}

void MeshBuilder::addFaceToMesh(uint32_t block, int blockType, int faceNum)
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
        for (int vertex = 0; vertex < 4; vertex++)
        {
            for (int element = 0; element < 3; element++)
            {
                m_waterVertices.push_back(
                    faceData.coords[vertex * 3 + element] + blockCoords[element] + 0.5f
                );
            }
            m_waterVertices.push_back(texCoords[vertex * 2]);
            m_waterVertices.push_back(texCoords[vertex * 2 + 1]);
            m_waterVertices.push_back(getSmoothSkyLight(
                worldBlockPos, faceData.coords + vertex * 3, faceData.lightingBlock
            ));
            m_waterVertices.push_back(getSmoothBlockLight(
                worldBlockPos, faceData.coords + vertex * 3, faceData.lightingBlock
            ));
        }

        //index buffer
        int trueNumVertices = m_waterVertices.size() / 7;
        m_waterIndices.push_back(trueNumVertices - 4);
        m_waterIndices.push_back(trueNumVertices - 3);
        m_waterIndices.push_back(trueNumVertices - 2);
        m_waterIndices.push_back(trueNumVertices - 2);
        m_waterIndices.push_back(trueNumVertices - 1);
        m_waterIndices.push_back(trueNumVertices - 4);
    }
    else
    {
        float texCoords[8];
        ResourcePack::getTextureCoordinates(
            texCoords, faceData.UVcoords, blockData.faceTextureIndices[faceNum]
        );
        for (int vertex = 0; vertex < 4; vertex++)
        {
            for (int element = 0; element < 3; element++)
            {
                m_vertices.push_back(
                    faceData.coords[vertex * 3 + element] + blockCoords[element] + 0.5f
                );
                // float sub = m_vertices[*m_numVertices] - (int)m_vertices[*m_numVertices];
                // if (sub > 0)
                // {
                //  std::cout << sub << "\n";
                // }
                // for (int i = element; i < *m_numVertices; i += 7)
                // {
                //     if (std::abs(m_vertices[i] - m_vertices[*m_numVertices]) < 0.5f && m_vertices[i] != m_vertices[*m_numVertices])
                //     {
                //         // std::cout << std::setprecision(100) << m_vertices[i] << "\n";
                //         // std::cout << std::setprecision(100) << m_vertices[*m_numVertices] << "\n";
                //         std::cout << blockCoords[element] << "\n";
                //      // std::cout << element << "\n";
                //     }
                // }
            }
            m_vertices.push_back(texCoords[vertex * 2]);
            m_vertices.push_back(texCoords[vertex * 2 + 1]);
            float ambientOcclusion = getAmbientOcclusion(worldBlockPos, faceData.coords + vertex *
                3, faceData.lightingBlock);
            m_vertices.push_back(getSmoothSkyLight(
                worldBlockPos, faceData.coords + vertex * 3, faceData.lightingBlock
            ) * ambientOcclusion);
            m_vertices.push_back(getSmoothBlockLight(
                worldBlockPos, faceData.coords + vertex * 3, faceData.lightingBlock
            ) * ambientOcclusion);
        }

        //index buffer
        int trueNumVertices = m_vertices.size() / 7;
        m_indices.push_back(trueNumVertices - 4);
        m_indices.push_back(trueNumVertices - 3);
        m_indices.push_back(trueNumVertices - 2);
        m_indices.push_back(trueNumVertices - 2);
        m_indices.push_back(trueNumVertices - 1);
        m_indices.push_back(trueNumVertices - 4);
    }
}

float MeshBuilder::getSmoothSkyLight(int* blockCoords, float* pointCoords, int direction)
{
    if (direction == 6)
    {
        return static_cast<float>(m_serverWorld.chunkManager.getSkyLight(blockCoords))
            / constants::skyLightMaxValue;
    }
    else
    {
        const int mask1[4] = { 0, 1, 0, 1 };
        const int mask2[4] = { 0, 0, 1, 1 };
        const int edges[4] = { 0, 1, 1, 0 };

        int cornerOffset[3];
        for (int i = 0; i < 3; i++)
        {
            cornerOffset[i] = ((pointCoords[i] - 0.499f) > 0) * 2 - 1;
        }

        int fixed = (s_neighbouringBlocksY[direction] != 0) + 2 * (s_neighbouringBlocksZ[direction]
            != 0);
        int unfixed1 = fixed == 0;
        int unfixed2 = 2 - (fixed == 2);

        int testBlockCoords[3];
        int normalDirection = (direction > 2) * 2 - 1;
        testBlockCoords[fixed] = blockCoords[fixed] + normalDirection;
        float brightness = 0;
        bool inShadow = false;
        int numTransparrentBlocks = 0;
        int numEdgesBlocked = 0;
        for (int i = 0; i < 4 && numEdgesBlocked < 2; i++)
        {
            testBlockCoords[unfixed1] = blockCoords[unfixed1] + mask1[i] * cornerOffset[unfixed1];
            testBlockCoords[unfixed2] = blockCoords[unfixed2] + mask2[i] * cornerOffset[unfixed2];
            bool transparrentBlock = m_serverWorld.getResourcePack().getBlockData(
                m_serverWorld.chunkManager.getBlock(testBlockCoords)).transparent;
            int cornerBrightness = m_serverWorld.chunkManager.getSkyLight(testBlockCoords)
                * transparrentBlock;
            testBlockCoords[fixed] += normalDirection;
            inShadow |= transparrentBlock && cornerBrightness < constants::skyLightMaxValue
                && m_serverWorld.chunkManager.getSkyLight(testBlockCoords) <= cornerBrightness;
            testBlockCoords[fixed] -= normalDirection;
            brightness += cornerBrightness;
            numTransparrentBlocks += transparrentBlock;
            numEdgesBlocked += !transparrentBlock * edges[i];
        }
        brightness = brightness/ numTransparrentBlocks;
        brightness = std::max(brightness - inShadow * 0.4f * std::sqrt(brightness), 0.0f);

        return brightness / constants::skyLightMaxValue;
    }
}

float MeshBuilder::getSmoothBlockLight(int* blockCoords, float* pointCoords, int direction)
{
    if (direction == 6)
    {
        return static_cast<float>(m_serverWorld.chunkManager.getBlockLight(blockCoords))
            / constants::blockLightMaxValue;
    }
    else
    {
        const int mask1[4] = { 0, 1, 0, 1 };
        const int mask2[4] = { 0, 0, 1, 1 };
        const int edges[4] = { 0, 1, 1, 0 };

        int cornerOffset[3];
        for (int i = 0; i < 3; i++)
        {
            cornerOffset[i] = ((pointCoords[i] - 0.499f) > 0) * 2 - 1;
        }

        int fixed = (s_neighbouringBlocksY[direction] != 0) + 2 * (s_neighbouringBlocksZ[direction]
            != 0);
        int unfixed1 = fixed == 0;
        int unfixed2 = 2 - (fixed == 2);

        int testBlockCoords[3];
        int normalDirection = (direction > 2) * 2 - 1;
        testBlockCoords[fixed] = blockCoords[fixed] + normalDirection;
        float brightness = 0;
        bool inShadow = false;
        int numTransparrentBlocks = 0;
        int numEdgesBlocked = 0;
        for (int i = 0; i < 4 && numEdgesBlocked < 2; i++)
        {
            testBlockCoords[unfixed1] = blockCoords[unfixed1] + mask1[i] * cornerOffset[unfixed1];
            testBlockCoords[unfixed2] = blockCoords[unfixed2] + mask2[i] * cornerOffset[unfixed2];
            bool transparrentBlock = m_serverWorld.getResourcePack().getBlockData(
                m_serverWorld.chunkManager.getBlock(testBlockCoords)).transparent;
            int cornerBrightness = m_serverWorld.chunkManager.getBlockLight(testBlockCoords)
                * transparrentBlock;
            testBlockCoords[fixed] += normalDirection;
            inShadow |= transparrentBlock && cornerBrightness < constants::blockLightMaxValue
                && m_serverWorld.chunkManager.getBlockLight(testBlockCoords) <= cornerBrightness;
            testBlockCoords[fixed] -= normalDirection;
            brightness += cornerBrightness;
            numTransparrentBlocks += transparrentBlock;
            numEdgesBlocked += !transparrentBlock * edges[i];
        }
        brightness = brightness/ numTransparrentBlocks;
        brightness = std::max(brightness - inShadow * 0.4f * std::sqrt(brightness), 0.0f);

        return brightness / constants::blockLightMaxValue;
    }
}

float MeshBuilder::getAmbientOcclusion(int* blockCoords, float* pointCoords, int direction)
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
        for (int i = 0; i < 3; i++)
        {
            cornerOffset[i] = ((pointCoords[i] - 0.499f) > 0) * 2 - 1;
        }

        int fixed = (s_neighbouringBlocksY[direction] != 0) + 2 * (s_neighbouringBlocksZ[direction] != 0);
        int unfixed1 = fixed == 0;
        int unfixed2 = 2 - (fixed == 2);

        int testBlockCoords[3];
        testBlockCoords[fixed] = blockCoords[fixed] + (direction > 2) * 2 - 1;
        int numSideOccluders = 0;
        int numCornerOccluders = 0;
        for (int i = 0; i < 3; i++)
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
    m_vertices.clear();
    m_indices.clear();
    m_waterVertices.clear();
    m_waterIndices.clear();

    int chunkPosition[3];
    m_chunk.getPosition(chunkPosition);
    int blockPos[3];
    int neighbouringBlockPos[3];
    int blockNum = 0;
    int layerNum = 0;
    for (blockPos[1] = chunkPosition[1] * constants::CHUNK_SIZE; blockPos[1] < (chunkPosition[1] + 1) * constants::CHUNK_SIZE; blockPos[1]++)
    {
        int layerBlockType = m_chunk.getLayerBlockType(layerNum);
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
                int blockType = m_chunk.getBlock(blockNum);
                if (blockType > 9)
                    LOG("Block type does not exist");
                if (blockType == 0)
                {
                    blockNum++;
                    continue;
                }
                if (blockType == 4)
                {
                    for (int faceNum = 0; faceNum < m_serverWorld.getResourcePack().
                        getBlockData(blockType).model->faces.size(); faceNum++)
                    {
                        int cullFace = m_serverWorld.getResourcePack().getBlockData(blockType).
                            model->faces[faceNum].cullFace;
                        neighbouringBlockPos[0] = blockPos[0] + s_neighbouringBlocksX[cullFace];
                        neighbouringBlockPos[1] = blockPos[1] + s_neighbouringBlocksY[cullFace];
                        neighbouringBlockPos[2] = blockPos[2] + s_neighbouringBlocksZ[cullFace];
                        int neighbouringBlockType = m_serverWorld.chunkManager.getBlock(neighbouringBlockPos);
                        if ((neighbouringBlockType != 4) && (m_serverWorld.getResourcePack().
                            getBlockData(neighbouringBlockType).transparent))
                        {
                            addFaceToMesh(blockNum, blockType, faceNum);
                        }
                    }
                }
                else
                {
                    auto& blockData = m_serverWorld.getResourcePack().getBlockData(blockType);
                    for (int faceNum = 0; faceNum < m_serverWorld.getResourcePack().
                        getBlockData(blockType).model->faces.size(); faceNum++)
                    {
                        int cullFace = m_serverWorld.getResourcePack().getBlockData(blockType).
                            model->faces.at(faceNum).cullFace;
                        if (cullFace < 0)
                            addFaceToMesh(blockNum, blockType, faceNum);
                        else
                        {
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
                }
                blockNum++;
            }
        }
        layerNum++;
    }
}

}  // namespace lonelycube::client
