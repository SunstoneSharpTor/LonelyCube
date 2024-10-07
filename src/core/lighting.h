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
    // Propagates the skylight through the chunk
    static void propagateSkyLight(Position chunkPosition,
        std::unordered_map<Position, Chunk>& worldChunks, bool* neighbouringChunksToBeRelit,
        ResourcePack& resourcePack, Position blockPosition =
        Position(constants::WORLD_BORDER_DISTANCE * 2, constants::WORLD_BORDER_DISTANCE * 2,
        constants::WORLD_BORDER_DISTANCE * 2));
    // Propagates the absence of skylight through the chunk
    static void propagateSkyDarkness(Position chunkPosition,
        std::unordered_map<Position, Chunk>& worldChunks, bool* neighbouringChunksToBeRelit,
        ResourcePack& resourcePack, Position blockPosition =
        Position(constants::WORLD_BORDER_DISTANCE * 2, constants::WORLD_BORDER_DISTANCE * 2,
        constants::WORLD_BORDER_DISTANCE * 2));
};