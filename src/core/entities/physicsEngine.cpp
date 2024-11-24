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

#include "core/entities/physicsEngine.h"

#include "core/entities/ECS.h"
#include "core/entities/ECSView.h"
#include "core/entities/components/meshComponent.h"
#include "core/entities/components/physicsComponent.h"
#include "core/entities/components/transformComponent.h"
#include "core/resourcePack.h"

PhysicsEngine::PhysicsEngine(ChunkManager& chunkManager, ECS& ecs, const ResourcePack& resourcePack)
    : m_chunkManager(chunkManager), m_ecs(ecs), m_resourcePack(resourcePack) {}

void PhysicsEngine::stepPhysics(float DT)
{
    for (EntityId entity : ECSView<PhysicsComponent>(m_ecs))
    {
        TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
        PhysicsComponent& physics = m_ecs.get<PhysicsComponent>(entity);
        transform.subBlockCoords.x += physics.velocity.x;

    }
}

bool PhysicsEngine::entityCollidingWithWorld(EntityId entity)
{
    const TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
    const Model* entityModel = m_ecs.get<MeshComponent>(entity).model;
    Vec3 minVertex(entityModel->boundingBoxVertices);
    Vec3 maxVertex(entityModel->boundingBoxVertices + 15);
    minVertex += transform.subBlockCoords;
    maxVertex += transform.subBlockCoords;
    IVec3 minBlock = IVec3(minVertex) + transform.blockCoords;
    IVec3 maxBlock = IVec3(maxVertex) + transform.blockCoords;

    bool colliding = false;
    IVec3 block;
    for (block.y = minVertex.y; block.y <= maxVertex.y && !colliding; block.y++)
    {
        for (block.x = minVertex.x; block.x <= maxVertex.x && !colliding; block.x++)
        {
            for (block.z = minVertex.z; block.z <= maxVertex.z && !colliding; block.z++)
            {
                colliding = m_resourcePack.getBlockData(m_chunkManager.getBlock(block)).collidable;
            }
        }
    }
    return colliding;
}
