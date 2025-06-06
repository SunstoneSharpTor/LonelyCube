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

#pragma once

#include "core/pch.h"

#include "core/chunk.h"
#include "core/packet.h"

namespace lonelycube {

class Compression {
public:
    static void compressChunk(Packet<uint8_t,
    9 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE>& compressedChunk,
    Chunk& chunk);
    static void decompressChunk(Packet<uint8_t,
    9 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE>& compressedChunk,
    Chunk& chunk);
    static void getChunkPosition(Packet<uint8_t,
    9 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE>& compressedChunk,
    IVec3& position);
};

}  // namespace lonelycube
