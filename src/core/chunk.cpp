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

const short Chunk::neighbouringBlocks[6] = { -(constants::CHUNK_SIZE * constants::CHUNK_SIZE), -constants::CHUNK_SIZE, -1, 1, constants::CHUNK_SIZE, (constants::CHUNK_SIZE * constants::CHUNK_SIZE) };

Chunk::Chunk(Position position) {
    m_skyLightUpToDate = false;
    m_calculatingSkylight = false;
    m_playerCount = 0;

    for (unsigned int layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        m_blocks[layerNum] = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_layerBlockTypes[layerNum] = 256;
    }
    m_skyLight = new unsigned char[(constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2];

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
    for (char i = 0; i < 3; i++) {
        coordinates[i] = m_position[i];
    }
}

void Chunk::unload()
{
    for (unsigned int layer = 0; layer < constants::CHUNK_SIZE; layer++)
    {
        delete[] m_blocks[layer];
    }
    delete[] m_skyLight;
    m_skyLight = new unsigned char[0];
}

void Chunk::clearSkyLight() {
    //reset all sky light values in the chunk to 0
    for (unsigned int blockNum = 0; blockNum < ((constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2); blockNum++) {
        m_skyLight[blockNum] = 0;
    }
}

void Chunk::setBlock(unsigned int block, unsigned char blockType)
{
    unsigned int layerNum = block / (constants::CHUNK_SIZE * constants::CHUNK_SIZE);
    if (m_layerBlockTypes[layerNum] == 256)
    {
        m_blocks[layerNum][block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)] = blockType;
    }
    else
    {
        if (blockType != m_layerBlockTypes[layerNum])
        {
            m_blocks[layerNum] = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE];
            for (unsigned int blockNum = 0; blockNum < (constants::CHUNK_SIZE *
                constants::CHUNK_SIZE); blockNum++)
            {
                m_blocks[layerNum][blockNum] = m_layerBlockTypes[layerNum];
            }
            m_blocks[layerNum][block % (constants::CHUNK_SIZE * constants::CHUNK_SIZE)] = blockType;
            m_layerBlockTypes[layerNum] = 256;
        }
    }
}

void Chunk::uncompressBlocks() {
    for (unsigned int layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerBlockTypes[layerNum] != 256)
        {
            m_blocks[layerNum] = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE];
            for (unsigned int blockNum = 0; blockNum < (constants::CHUNK_SIZE *
                constants::CHUNK_SIZE); blockNum++)
            {
                m_blocks[layerNum][blockNum] = m_layerBlockTypes[layerNum];
            }
            m_layerBlockTypes[layerNum] = 256;
        }
    }
}

void Chunk::compressBlocks() {
    for (unsigned int layerNum = 0; layerNum < constants::CHUNK_SIZE; layerNum++)
    {
        if (m_layerBlockTypes[layerNum] == 256)
        {
            bool simpleLayer = true;
            for (unsigned int blockNum = 1; blockNum < (constants::CHUNK_SIZE *
                constants::CHUNK_SIZE) && simpleLayer; blockNum++)
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