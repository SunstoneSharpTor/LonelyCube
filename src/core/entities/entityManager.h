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
#include "core/entities/physicsEngine.h"
#include "core/resourcePack.h"
#include "core/utils/iVec3.h"
#include "core/utils/vec3.h"

namespace lonelycube {

class EntityManager {
private:
    ECS m_ecs;
    ChunkManager& m_chunkManager;
    const ResourcePack& m_resourcePack;

    PhysicsEngine m_physicsEngine;

    void tickItems();

public:
    EntityManager(int maxNumEntities, ChunkManager& chunkManager, const ResourcePack& resourcePack);
    void tick();
    void addItem(uint8_t blockType, IVec3 blockPosition, Vec3 subBlockPosition);
    inline ECS& getECS()
    {
        return m_ecs;
    }
    inline PhysicsEngine& getPhysicsEngine()
    {
        return m_physicsEngine;
    }
};

}  // namespace lonelycube
