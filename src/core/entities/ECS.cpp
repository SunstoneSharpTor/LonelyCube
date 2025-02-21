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

#include "core/entities/ECS.h"

namespace lonelycube {

ECS::ECS(int maxEntities) : m_maxEntities(maxEntities) {}

ECS::~ECS() {
    for (ComponentPool* pool : m_componentPools) {
        delete pool;
    }
}

EntityId ECS::newEntity()
{
    if (m_freeEntities.empty())
    {
        EntityId id = createEntityId(m_entities.size(), 0);
        m_entities.emplace_back(id, ComponentMask());
        return id;
    }
    EntityIndex index = m_freeEntities.back();
    m_freeEntities.pop_back();
    EntityId id = createEntityId(index, getEntityVersion(m_entities[index].id));
    m_entities[index].id = id;
    return id;
}

void ECS::destroyEntity(const EntityId id)
{
    EntityId newId = createEntityId(EntityIndex(-1), getEntityVersion(id) + 1);
    m_entities[getEntityIndex(id)].id = newId;
    m_entities[getEntityIndex(id)].mask.reset();
    m_freeEntities.push_back(getEntityIndex(id));
}

}  // namespace lonelycube
