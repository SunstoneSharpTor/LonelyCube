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

#include "core/chunk.h"
#include "core/utils/iVec3.h"

namespace lonelycube {

class ChunkManager
{
private:
    std::unordered_map<IVec3, Chunk> m_chunks;

public:
    ChunkManager();
    uint8_t getBlock(const IVec3& position) const;
    void setBlock(const IVec3& position, uint8_t blockType);
    uint8_t getSkyLight(const IVec3& position) const;
    uint8_t getBlockLight(const IVec3& position) const;
    inline Chunk& getChunk(const IVec3& chunkPosition) {
        return m_chunks.at(chunkPosition);
    }
    inline bool chunkLoaded(const IVec3& chunkPosition) {
        return m_chunks.contains(chunkPosition);
    }
    inline std::unordered_map<IVec3, Chunk>& getWorldChunks() {
        return m_chunks;
    }
};

}  // namespace lonelycube
