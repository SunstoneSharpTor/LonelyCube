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

#include "core/entities/entityManager.h"

#include "core/chunkManager.h"
#include "core/constants.h"
#include "core/entities/ECS.h"
#include "core/entities/ECSView.h"
#include "core/entities/components/itemComponent.h"
#include "core/entities/components/meshComponent.h"
#include "core/entities/components/physicsComponent.h"
#include "core/entities/components/transformComponent.h"
#include "core/random.h"

namespace lonelycube {

EntityManager::EntityManager(int maxNumEntities, ChunkManager& chunkManager, const ResourcePack&
    resourcePack)
    : m_ecs(maxNumEntities), m_chunkManager(chunkManager), m_resourcePack(resourcePack),
    m_physicsEngine(chunkManager, m_ecs, m_resourcePack) {}

void EntityManager::addItem(uint8_t blockType, IVec3 blockCoords, Vec3 subBlockCoords)
{
    std::lock_guard<std::mutex> lock(m_ecs.mutex);
    EntityId entity = m_ecs.newEntity();
    m_ecs.assign<TransformComponent>(
        entity, blockCoords, subBlockCoords, 0.25f, Vec3(0.0f, 1.0f, 0.0f)
    );
    Vec3 initialVelocity;
    initialVelocity.x = std::fmod((PCG_Random32() * 0.0001f), 8.0f) - 4.0f;
    initialVelocity.y = std::fmod((PCG_Random32() * 0.0001f), 3.0f) + 3.5f;
    initialVelocity.z = std::fmod((PCG_Random32() * 0.0001f), 8.0f) - 4.0f;
    m_ecs.assign<PhysicsComponent>(entity, initialVelocity, Vec3(0.0f, -0.5f, 0.0f));
    const Model* blockModel = m_resourcePack.getBlockData(blockType).model;
    const uint16_t* textureIndices = m_resourcePack.getBlockData(blockType).faceTextureIndices;
    m_ecs.assign<MeshComponent>(entity, blockModel, textureIndices);
    m_ecs.assign<ItemComponent>(entity, 3600 * constants::TICKS_PER_SECOND);
}

void EntityManager::tickItems()
{
    for (EntityId entity : ECSView<ItemComponent>(m_ecs))
    {
        ItemComponent& itemComponent = m_ecs.get<ItemComponent>(entity);
        if (--itemComponent.timer == 0)
            m_ecs.destroyEntity(entity);
    }
}

void EntityManager::tick()
{
    std::lock_guard<std::mutex> lock1(m_ecs.mutex);
    tickItems();
    m_physicsEngine.stepPhysics();
}

}  // namespace lonelycube
