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

#pragma once

#include "core/pch.h"

#include "core/chunk.h"
#include "core/position.h"
#include "core/resourcePack.h"

class Lighting {
public:
    // Propagates the sky light through the chunk
    static void propagateSkyLight(Position chunkPosition, std::unordered_map<Position, Chunk>&
        worldChunks, bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, ResourcePack&
        resourcePack, uint32_t modifiedBlock = constants::CHUNK_SIZE * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE);

    // Propagates the block light through the chunk
    static void propagateBlockLight(Position chunkPosition, std::unordered_map<Position, Chunk>&
        worldChunks, bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, ResourcePack&
        resourcePack, uint32_t modifiedBlock = constants::CHUNK_SIZE * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE);

    // Propagates the absence of sky light through the chunk
    static void propagateSkyDarkness(Position chunkPosition, std::unordered_map<Position, Chunk>&
        worldChunks, bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, ResourcePack&
        resourcePack, uint32_t modifiedBlock = constants::CHUNK_SIZE * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE);

    // Propagates the absence of block light through the chunk
    static void propagateBlockDarkness(Position chunkPosition, std::unordered_map<Position, Chunk>&
        worldChunks, bool* neighbouringChunksToBeRelit, bool* chunksToRemesh, ResourcePack&
        resourcePack, uint32_t modifiedBlock = constants::CHUNK_SIZE * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE);

    // Recalculate all necessary lighting information for a block change
	static void relightChunksAroundBlock(const Position& blockCoords, const Position& chunkPosition,
		uint8_t originalBlock, uint8_t newBlock, std::vector<Position>& chunksToRemesh,
        std::unordered_map<Position, Chunk>& worldChunks, ResourcePack& ResourcePack);
private:
    static const inline std::array<Position, 6> s_neighbouringChunkOffsets = { Position(0, -1, 0),
        Position(0, 0, -1),
        Position(-1, 0, 0),
        Position(1, 0, 0),
        Position(0, 0, 1),
        Position(0, 1, 0) };
};