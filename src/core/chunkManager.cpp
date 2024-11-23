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

#include "core/chunkManager.h"

ChunkManager::ChunkManager()
{
    // TODO:
    // set this reserved number for m_chunks to be dependant on render distance for singleplayer
    m_chunks.reserve(16777214);
}

uint8_t ChunkManager::getBlock(const IVec3& position) const {
    IVec3 chunkPosition = Chunk::getChunkCoords(position);
    IVec3 chunkBlockCoords = IVec3(
        position.x - chunkPosition.x * constants::CHUNK_SIZE,
        position.y - chunkPosition.y * constants::CHUNK_SIZE,
        position.z - chunkPosition.z * constants::CHUNK_SIZE);
    uint32_t chunkBlockNum = chunkBlockCoords.y * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        + chunkBlockCoords.z * constants::CHUNK_SIZE + chunkBlockCoords.x;

    auto chunkIterator = m_chunks.find(chunkPosition);

    if (chunkIterator == m_chunks.end()) {
        return 0;
    }

    return chunkIterator->second.getBlock(chunkBlockNum);
}

void ChunkManager::setBlock(const IVec3& position, uint8_t blockType) {
    IVec3 chunkPosition = Chunk::getChunkCoords(position);
    IVec3 chunkBlockCoords = IVec3(
        position.x - chunkPosition.x * constants::CHUNK_SIZE,
        position.y - chunkPosition.y * constants::CHUNK_SIZE,
        position.z - chunkPosition.z * constants::CHUNK_SIZE);
    uint32_t chunkBlockNum = chunkBlockCoords.y * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        + chunkBlockCoords.z * constants::CHUNK_SIZE + chunkBlockCoords.x;

    auto chunkIterator = m_chunks.find(chunkPosition);

    if (chunkIterator == m_chunks.end()) {
        return;
    }

    chunkIterator->second.setBlock(chunkBlockNum, blockType);
    chunkIterator->second.compressBlocks();
}

uint8_t ChunkManager::getSkyLight(const IVec3& position) const {
    IVec3 chunkPosition = Chunk::getChunkCoords(position);
    IVec3 chunkBlockCoords = IVec3(
        position.x - chunkPosition.x * constants::CHUNK_SIZE,
        position.y - chunkPosition.y * constants::CHUNK_SIZE,
        position.z - chunkPosition.z * constants::CHUNK_SIZE);
    uint32_t chunkBlockNum = chunkBlockCoords.y * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        + chunkBlockCoords.z * constants::CHUNK_SIZE + chunkBlockCoords.x;

    auto chunkIterator = m_chunks.find(chunkPosition);

    if (chunkIterator == m_chunks.end()) {
        return 0;
    }

    return chunkIterator->second.getSkyLight(chunkBlockNum);
}

uint8_t ChunkManager::getBlockLight(const IVec3& position) const {
    IVec3 chunkPosition = Chunk::getChunkCoords(position);
    IVec3 chunkBlockCoords = IVec3(
        position.x - chunkPosition.x * constants::CHUNK_SIZE,
        position.y - chunkPosition.y * constants::CHUNK_SIZE,
        position.z - chunkPosition.z * constants::CHUNK_SIZE);
    uint32_t chunkBlockNum = chunkBlockCoords.y * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        + chunkBlockCoords.z * constants::CHUNK_SIZE + chunkBlockCoords.x;

    auto chunkIterator = m_chunks.find(chunkPosition);

    if (chunkIterator == m_chunks.end()) {
        return 0;
    }

    return chunkIterator->second.getBlockLight(chunkBlockNum);
}
