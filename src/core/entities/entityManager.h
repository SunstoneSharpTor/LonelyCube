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

#include "client/entities/components/meshComponent.h"
#include "core/entities/components/transformComponent.h"

#include "core/entities/ECS.h"
#include "core/utils/iVec3.h"
#include "core/resourcePack.h"
#include "core/utils/vec3.h"

template<bool integrated>
class EntityManager {
private:
    const ResourcePack& m_resourcePack;

public:
    ECS ecs;

    EntityManager(int maxNumEntities, const ResourcePack& resourcePack);
    void addItem(uint8_t blockType, IVec3 blockPosition, Vec3 subBlockPosition);
};

template<bool integrated>
EntityManager<integrated>::EntityManager(int maxNumEntities, const ResourcePack& resourcePack)
    : ecs(maxNumEntities), m_resourcePack(resourcePack) {}

template<bool integrated>
void EntityManager<integrated>::addItem(uint8_t blockType, IVec3 blockPosition, Vec3 subBlockPosition)
{
    EntityId entity = ecs.newEntity();
    ecs.assign<TransformComponent>(entity);
    if (integrated)
        ecs.assign<MeshComponent>(entity);
}
