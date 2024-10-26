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

#include "core/chunk.h"

#include "core/pch.h"

#include "core/position.h"

std::mutex Chunk::s_checkingNeighbouringRelights;

const int16_t Chunk::neighbouringBlocks[6] = { -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -constants::CHUNK_SIZE, -1, 1, constants::CHUNK_SIZE, (constants::CHUNK_SIZE * constants::CHUNK_SIZE) };

Chunk::Chunk(Position position) {
    m_skyLightUpToDate = false;
    m_calculatingSkylight = false;
    m_playerCount = 0;

    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        m_blocks[layerNum] = new uint8_t[constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_layerBlockTypes[layerNum] = 256;
        m_skyLight[layerNum] = new uint8_t[(constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];
        m_layerSkyLightValues[layerNum] = constants::skyLightMaxValue + 1;
    }

    m_position[0] = position.x;
    m_position[1] = position.y;
    m_position[2] = position.z;
}

Chunk::Chunk() {
    m_skyLightUpToDate = false;
    m_calculatingSkylight = false;
    m_playerCount = 0;

    m_position[0] = 0;
    m_position[1] = 0;
    m_position[2] = 0;
}

void Chunk::getChunkPosition(int* coordinates) const {
    for (int8_t i = 0; i < 3; i++) {
        coordinates[i] = m_position[i];
    }
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
    }
}

void Chunk::clearSkyLight()
{
    //reset all sky light values in the chunk to 0
    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerSkyLightValues[layerNum] != constants::skyLightMaxValue + 1)
        {
            m_layerSkyLightValues[layerNum] = constants::skyLightMaxValue + 1;
            m_skyLight[layerNum] = new uint8_t[(constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];
        }
        for (uint32_t blockNum = 0; blockNum < ((constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2); blockNum++)
        {
            m_skyLight[layerNum][blockNum] = 0;
        }
    }
}

void Chunk::clearBlocksAndLight() {
    //reset all sky light values in the chunk to 0
    for (uint32_t layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerBlockTypes[layerNum] != 256)
        {
            m_layerBlockTypes[layerNum] = 256;
            m_blocks[layerNum] = new uint8_t[constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        }
        if (m_layerSkyLightValues[layerNum] != constants::skyLightMaxValue + 1)
        {
            m_layerSkyLightValues[layerNum] = constants::skyLightMaxValue + 1;
            m_skyLight[layerNum] = new uint8_t[(constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];
        }
        for (uint32_t blockNum = 0; blockNum < constants::CHUNK_SIZE * constants::CHUNK_SIZE; blockNum++)
        {
            m_blocks[layerNum][blockNum] = 0;
        }
        for (uint32_t blockNum = 0; blockNum < ((constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2); blockNum++)
        {
            m_skyLight[layerNum][blockNum] = 0;
        }
    }
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

void Chunk::compressBlocksAndLight()
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
    compressSkyLight();
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
    }
}