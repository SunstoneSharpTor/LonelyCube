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

#include "core/chunk.h"

#include "core/constants.h"
#include "core/pch.h"

#include "core/block.h"
#include "core/utils/iVec3.h"

std::mutex Chunk::s_checkingNeighbouringRelights;

const int16_t Chunk::neighbouringBlocks[6] = { -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -constants::CHUNK_SIZE, -1, 1, constants::CHUNK_SIZE, (constants::CHUNK_SIZE * constants::CHUNK_SIZE) };

Chunk::Chunk(IVec3 position) : m_position(position) {
    m_skyLightUpToDate = false;
    m_blockLightUpToDate = true;
    m_calculatingSkylight = false;
    m_playerCount = 0;

    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        m_blocks[layerNum] = new uint8_t[constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_layerBlockTypes[layerNum] = 256;
        m_layerSkyLightValues[layerNum] = 0;
        m_layerBlockLightValues[layerNum] = 0;
    }
}

Chunk::Chunk() {
    m_skyLightUpToDate = false;
    m_blockLightUpToDate = true;
    m_calculatingSkylight = false;
    m_playerCount = 0;
}

void Chunk::getPosition(int* coordinates) const {
    coordinates[0] = m_position.x;
    coordinates[1] = m_position.y;
    coordinates[2] = m_position.z;
}

void Chunk::unload()
{
    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerBlockTypes[layerNum] == 256)
        {
            delete[] m_blocks[layerNum];
        }
        if (m_layerSkyLightValues[layerNum] == constants::skyLightMaxValue + 1)
        {
            delete[] m_skyLight[layerNum];
        }
        if (m_layerBlockLightValues[layerNum] == constants::blockLightMaxValue + 1)
        {
            delete[] m_blockLight[layerNum];
        }
    }
}

void Chunk::clearSkyLight()
{
    // Reset all sky light values in the chunk to 0
    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerSkyLightValues[layerNum] == constants::skyLightMaxValue + 1)
        {
            delete[] m_skyLight[layerNum];
        }
        m_layerSkyLightValues[layerNum] = 0;
    }
}

void Chunk::clearBlockLight()
{
    // Reset all block light values in the chunk to 0
    // Default the block light to be 0 as it is unlikely to be greater than 0 for naturally
    // generated terrain
    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerBlockLightValues[layerNum] == constants::blockLightMaxValue + 1)
        {
            delete[] m_blockLight[layerNum];
        }
        m_layerBlockLightValues[layerNum] = 0;
    }
}

void Chunk::clearBlocksAndLight()
{
    // Set all blocks in the chunk to air
    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerBlockTypes[layerNum] != 256)
        {
            m_layerBlockTypes[layerNum] = 256;
            m_blocks[layerNum] = new uint8_t[constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        }
        for (uint32_t blockNum = 0; blockNum < constants::CHUNK_SIZE * constants::CHUNK_SIZE; blockNum++)
        {
            m_blocks[layerNum][blockNum] = air;
        }
    }
    clearSkyLight();
    clearBlockLight();
}

void Chunk::setBlock(uint32_t block, uint8_t blockType)
{
    uint32_t layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
    if (m_layerBlockTypes[layerNum] == 256)
    {
        m_blocks[layerNum][block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)] = blockType;
    }
    else
    {
        if (blockType != m_layerBlockTypes[layerNum])
        {
            m_blocks[layerNum] = new uint8_t[constants::CHUNK_SIZE * constants::CHUNK_SIZE];
            for (uint32_t blockNum = 0; blockNum < (constants::CHUNK_SIZE *
                constants::CHUNK_SIZE); blockNum++)
            {
                m_blocks[layerNum][blockNum] = m_layerBlockTypes[layerNum];
            }
            m_blocks[layerNum][block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)] = blockType;
            m_layerBlockTypes[layerNum] = 256;
        }
    }
}

void Chunk::setSkyLight(uint32_t block, uint8_t value)
{
    uint32_t layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
    if (m_layerSkyLightValues[layerNum] == constants::skyLightMaxValue + 1)
    {
        bool oddBlockNum = block % 2;
        bool evenBlockNum = !oddBlockNum;
        uint32_t index = block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE) / 2;
        m_skyLight[layerNum][index] &= 0b00001111 << (4 * evenBlockNum);
        m_skyLight[layerNum][index] |= value << (4 * oddBlockNum);
    }
    else
    {
        if (value != m_layerSkyLightValues[layerNum])
        {
            m_skyLight[layerNum] =
                new uint8_t[(constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];
            uint8_t doubledUpValue = (m_layerSkyLightValues[layerNum] << 4) +
                m_layerSkyLightValues[layerNum];
            for (uint32_t blockNum = 0; blockNum < ((constants::CHUNK_SIZE *
                constants::CHUNK_SIZE + 1) / 2); blockNum++)
            {
                m_skyLight[layerNum][blockNum] = doubledUpValue;
            }
            bool oddBlockNum = block % 2;
            bool evenBlockNum = !oddBlockNum;
            uint32_t index = block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE) / 2;
            m_skyLight[layerNum][index] &= 0b00001111 << (4 * evenBlockNum);
            m_skyLight[layerNum][index] |= value << (4 * oddBlockNum);
            m_layerSkyLightValues[layerNum] = constants::skyLightMaxValue + 1;
        }
    }
}

void Chunk::setBlockLight(uint32_t block, uint8_t value)
{
    uint32_t layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
    if (m_layerBlockLightValues[layerNum] == constants::blockLightMaxValue + 1)
    {
        bool oddBlockNum = block % 2;
        bool evenBlockNum = !oddBlockNum;
        uint32_t index = block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE) / 2;
        m_blockLight[layerNum][index] &= 0b00001111 << (4 * evenBlockNum);
        m_blockLight[layerNum][index] |= value << (4 * oddBlockNum);
    }
    else
    {
        if (value != m_layerBlockLightValues[layerNum])
        {
            m_blockLight[layerNum] =
                new uint8_t[(constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];
            uint8_t doubledUpValue = (m_layerBlockLightValues[layerNum] << 4) +
                m_layerBlockLightValues[layerNum];
            for (uint32_t blockNum = 0; blockNum < ((constants::CHUNK_SIZE *
                constants::CHUNK_SIZE + 1) / 2); blockNum++)
            {
                m_blockLight[layerNum][blockNum] = doubledUpValue;
            }
            bool oddBlockNum = block % 2;
            bool evenBlockNum = !oddBlockNum;
            uint32_t index = block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE) / 2;
            m_blockLight[layerNum][index] &= 0b00001111 << (4 * evenBlockNum);
            m_blockLight[layerNum][index] |= value << (4 * oddBlockNum);
            m_layerBlockLightValues[layerNum] = constants::blockLightMaxValue + 1;
        }
    }
}

void Chunk::compressBlocks()
{
    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerBlockTypes[layerNum] == 256)
        {
            bool simpleLayer = true;
            for (uint32_t blockNum = 1; blockNum < constants::CHUNK_SIZE *
                constants::CHUNK_SIZE && simpleLayer; blockNum++)
            {
                simpleLayer &= m_blocks[layerNum][blockNum] == m_blocks[layerNum][0];
            }
            if (simpleLayer)
            {
                m_layerBlockTypes[layerNum] = m_blocks[layerNum][0];
                delete[] m_blocks[layerNum];
            }
        }
    }
}

void Chunk::compressSkyLight()
{
    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerSkyLightValues[layerNum] == constants::skyLightMaxValue + 1)
        {
            bool simpleLayer = (m_skyLight[layerNum][0] & 0b1111) == m_skyLight[layerNum][0] >> 4;
            for (uint32_t blockNum = 1; blockNum < (constants::CHUNK_SIZE *
                constants::CHUNK_SIZE + 1) / 2 && simpleLayer; blockNum++)
            {
                simpleLayer &= m_skyLight[layerNum][blockNum] == m_skyLight[layerNum][0];
            }
            if (simpleLayer)
            {
                m_layerSkyLightValues[layerNum] = m_skyLight[layerNum][0] >> 4;
                delete[] m_skyLight[layerNum];
            }
        }
    }
}

void Chunk::compressBlockLight()
{
    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerBlockLightValues[layerNum] == constants::blockLightMaxValue + 1)
        {
            bool simpleLayer = (m_blockLight[layerNum][0] & 0b1111) == m_blockLight[layerNum][0] >> 4;
            for (uint32_t blockNum = 1; blockNum < (constants::CHUNK_SIZE *
                constants::CHUNK_SIZE + 1) / 2 && simpleLayer; blockNum++)
            {
                simpleLayer &= m_blockLight[layerNum][blockNum] == m_blockLight[layerNum][0];
            }
            if (simpleLayer)
            {
                m_layerBlockLightValues[layerNum] = m_blockLight[layerNum][0] >> 4;
                delete[] m_blockLight[layerNum];
            }
        }
    }
}

void Chunk::compressBlocksAndLight()
{
    compressBlocks();
    compressSkyLight();
    compressBlockLight();
}

void Chunk::uncompressBlocksAndLight()
{
    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerBlockTypes[layerNum] != 256)
        {
            m_blocks[layerNum] = new uint8_t[constants::CHUNK_SIZE * constants::CHUNK_SIZE];
            for (uint32_t blockNum = 0; blockNum < constants::CHUNK_SIZE *
                constants::CHUNK_SIZE; blockNum++)
            {
                m_blocks[layerNum][blockNum] = m_layerBlockTypes[layerNum];
            }
            m_layerBlockTypes[layerNum] = 256;
        }
        if (m_layerSkyLightValues[layerNum] != constants::skyLightMaxValue + 1)
        {
            m_skyLight[layerNum] = new uint8_t[(constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];
            uint8_t doubledUpValue = (m_layerSkyLightValues[layerNum] << 4) +
                m_layerSkyLightValues[layerNum];
            for (uint32_t blockNum = 0; blockNum < (constants::CHUNK_SIZE *
                constants::CHUNK_SIZE + 1) / 2; blockNum++)
            {
                m_skyLight[layerNum][blockNum] = doubledUpValue;
            }
            m_layerSkyLightValues[layerNum] = constants::skyLightMaxValue + 1;
        }
        if (m_layerBlockLightValues[layerNum] != constants::blockLightMaxValue + 1)
        {
            m_blockLight[layerNum] = new uint8_t[(constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];
            uint8_t doubledUpValue = (m_layerBlockLightValues[layerNum] << 4) +
                m_layerBlockLightValues[layerNum];
            for (uint32_t blockNum = 0; blockNum < (constants::CHUNK_SIZE *
                constants::CHUNK_SIZE + 1) / 2; blockNum++)
            {
                m_blockLight[layerNum][blockNum] = doubledUpValue;
            }
            m_layerBlockLightValues[layerNum] = constants::blockLightMaxValue + 1;
        }
    }
}
