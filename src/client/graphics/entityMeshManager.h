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

#include "core/entities/ECS.h"
#include "core/serverWorld.h"
#include "core/utils/iVec3.h"
#include "core/utils/vec3.h"

namespace lonelycube {

class EntityMeshManager
{
private:
    ECS& m_ecs;
    ServerWorld<true>& m_serverWorld;

    float interpolateSkyLight(const IVec3& blockPosition, const Vec3& subBlockPosition);
    float interpolateBlockLight(const IVec3& blockPosition, const Vec3& subBlockPosition);

public:
    uint32_t numIndices;
    uint32_t sizeOfVertices;

    EntityMeshManager(ServerWorld<true>& serverWorld);
    void createBatch(
        const IVec3 playerBlockCoords, float* vertexBuffer, uint32_t* indexBuffer,
        const float timeSinceLastTick
    );
};

}  // namespace lonelycube
