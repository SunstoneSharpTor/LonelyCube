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

Chunk::Chunk() {
    inUse = false;
    m_singleBlockType = false;
    m_singleSkyLightVal = false;
    m_skyLightUpToDate = false;
    m_calculatingSkylight = false;
    m_playerCount = 0;

    m_blocks = new unsigned char[0];
    m_skyLight = new unsigned char[0];

    m_position[0] = 0;
    m_position[1] = 0;
    m_position[2] = 0;
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

void Chunk::clearSkyLight() {
    //reset all sky light values in the chunk to 0
    for (unsigned int blockNum = 0; blockNum < ((constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE + 1) / 2); blockNum++) {
        m_skyLight[blockNum] = 0;
    }
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
    // setSkyLight(block, 0 * !(constants::castsShadows[blockType]));
}

void Chunk::clearBlocksAndLight() {
    //reset all blocks in the chunk to air and all skylight values to 0
    for (unsigned int blockNum = 0; blockNum < constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE; blockNum++) {
        m_blocks[blockNum] = 0;
        m_skyLight[blockNum >> 1] = 0;
    }
}

void Chunk::uncompressBlocks() {
    if (m_singleBlockType) {
        delete m_blocks;
        m_blocks = new unsigned char[constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        for (unsigned int block = 0; block < (constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE); block++) {
            m_blocks[block] = m_blocks[0];
        }
    }
}

void Chunk::compressBlocks() {
    if (m_singleBlockType) {
        unsigned char blockType = m_blocks[0];
        delete[] m_blocks;
        m_blocks = new unsigned char[1];
        m_blocks[0] = blockType;
    }
}