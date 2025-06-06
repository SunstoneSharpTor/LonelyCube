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

#include "core/chunkManager.h"
#include "core/entities/ECS.h"
#include "core/resourcePack.h"

namespace lonelycube {

class PhysicsEngine
{
private:
    ChunkManager& m_chunkManager;
    ECS& m_ecs;
    const ResourcePack& m_resourcePack;

    void stepPhysics(const EntityId entity, const float DT);
    bool entityCollidingWithWorld(const EntityId entity);
    float findPenetrationDepthIntoWorld(
        const EntityId entity, const int axis, const float displacementAlongAxis
    );

public:
    PhysicsEngine(ChunkManager& chunkManager, ECS& ecs, const ResourcePack& resourcePack);
    void stepPhysics();
    void extrapolateTransforms(const float DT);
};

}  // namespace lonelycube
