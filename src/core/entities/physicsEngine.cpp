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

#include "core/constants.h"
#include "core/entities/ECS.h"
#include "core/entities/ECSView.h"
#include "core/entities/components/meshComponent.h"
#include "core/entities/components/physicsComponent.h"
#include "core/entities/components/transformComponent.h"
#include "core/resourcePack.h"

PhysicsEngine::PhysicsEngine(ChunkManager& chunkManager, ECS& ecs, const ResourcePack& resourcePack)
    : m_chunkManager(chunkManager), m_ecs(ecs), m_resourcePack(resourcePack) {}

void PhysicsEngine::stepPhysics()
{
    const float DT = 1.0f / constants::TICKS_PER_SECOND;
    for (EntityId entity : ECSView<TransformComponent, PhysicsComponent>(m_ecs))
    {
        TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
        PhysicsComponent& physics = m_ecs.get<PhysicsComponent>(entity);
        physics.velocity.y -= 12.0f * DT;
        physics.velocity *= 0.8f;
        transform.rotation += physics.angularVelocity * DT;
        for (int axis = 0; axis < 3; axis++)
        {
            transform.subBlockCoords[axis] += physics.velocity[axis] * DT;
            if (entityCollidingWithWorld(entity))
            {
                transform.subBlockCoords[axis] -= physics.velocity[axis] * DT;
                physics.velocity[axis] = 0.0f;
            }
            else
            {
                int carry = static_cast<int>(std::floor(transform.subBlockCoords[axis]));
                transform.subBlockCoords[axis] -= carry;
                transform.blockCoords[axis] += carry;
            }
        }
        transform.updateTransform();

    }
}

bool PhysicsEngine::entityCollidingWithWorld(EntityId entity)
{
    const TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
    const Model* entityModel = m_ecs.get<MeshComponent>(entity).model;
    Vec3 minVertex(entityModel->boundingBoxVertices);
    Vec3 maxVertex(entityModel->boundingBoxVertices + 15);
    minVertex = minVertex * transform.scale + transform.subBlockCoords;
    maxVertex = maxVertex * transform.scale + transform.subBlockCoords;
    IVec3 minBlock = IVec3(minVertex) + transform.blockCoords;
    IVec3 maxBlock = IVec3(maxVertex) + transform.blockCoords;

    bool colliding = false;
    IVec3 block;
    for (block.y = minBlock.y; block.y <= maxBlock.y && !colliding; block.y++)
    {
        for (block.x = minBlock.x; block.x <= maxBlock.x && !colliding; block.x++)
        {
            for (block.z = minBlock.z; block.z <= maxBlock.z && !colliding; block.z++)
            {
                colliding = m_resourcePack.getBlockData(m_chunkManager.getBlock(block)).collidable;
            }
        }
    }
    return colliding;
}

void PhysicsEngine::extrapolateTransforms(float DT)
{
    for (EntityId entity : ECSView<TransformComponent, PhysicsComponent>(m_ecs))
    {
        TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
        PhysicsComponent& physics = m_ecs.get<PhysicsComponent>(entity);
        Vec3 actualSubBlockCoords = transform.subBlockCoords;
        Vec3 actualRotation = transform.rotation;
        Vec3 averageVelocity = physics.velocity;
        averageVelocity.y -= 12.0f / constants::TICKS_PER_SECOND * physics.velocity.y > 0;
        averageVelocity *= 0.8f;
        transform.subBlockCoords += averageVelocity * DT;
        transform.rotation += physics.angularVelocity * DT;
        transform.updateTransform();
        transform.subBlockCoords = actualSubBlockCoords;
        transform.rotation = actualRotation;
    }
}
