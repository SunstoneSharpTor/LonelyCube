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

#include "core/entities/physicsEngine.h"

#include "core/constants.h"
#include "core/entities/ECS.h"
#include "core/entities/ECSView.h"
#include "core/entities/components/meshComponent.h"
#include "core/entities/components/physicsComponent.h"
#include "core/entities/components/transformComponent.h"
#include "core/log.h"
#include "core/random.h"
#include "core/resourcePack.h"

namespace lonelycube {

PhysicsEngine::PhysicsEngine(
    const ChunkManager& chunkManager, ECS& ecs, const ResourcePack& resourcePack
) : m_chunkManager(chunkManager), m_ecs(ecs), m_resourcePack(resourcePack) {}

void PhysicsEngine::stepPhysics(const EntityId entity, const float DT
) {
    TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
    PhysicsComponent& physics = m_ecs.get<PhysicsComponent>(entity);
    transform.rotation += physics.angularVelocity * DT;
    if (entityCollidingWithWorld(entity))
    {
        transform.subBlockCoords.y += physics.velocity.y * DT;
        int carry = static_cast<int>(std::floor(transform.subBlockCoords.y));
        transform.subBlockCoords.y -= carry;
        transform.blockCoords.y += carry;
        return;
    }

    physics.velocity.y -= 20.0f * DT;
    physics.velocity *= 0.98f;
    for (int axis = 0; axis < 3; axis++)
    {
        transform.subBlockCoords[axis] += physics.velocity[axis] * DT;
        if (entityCollidingWithWorld(entity))
        {
            if (physics.velocity[axis] > 0)
                transform.subBlockCoords[axis] -= findPenetrationDepthIntoWorld(entity, axis, 1);
            else
                transform.subBlockCoords[axis] += findPenetrationDepthIntoWorld(entity, axis, -1);

            physics.velocity[axis] = 0.0f;
            if (axis == 1)
            {
                physics.velocity[0] *= 0.6f;
                physics.velocity[2] *= 0.6f;
            }
        }
        else
        {
            int carry = static_cast<int>(std::floor(transform.subBlockCoords[axis]));
            transform.subBlockCoords[axis] -= carry;
            transform.blockCoords[axis] += carry;
        }
    }
}

void PhysicsEngine::stepPhysics()
{
    const float DT = 1.0f / constants::TICKS_PER_SECOND;
    for (EntityId entity : ECSView<TransformComponent, PhysicsComponent>(m_ecs))
    {
        stepPhysics(entity, DT);
        TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
        transform.updateTransformMatrix();
    }
}

bool PhysicsEngine::entityCollidingWithWorld(const EntityId entity)
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

float PhysicsEngine::findPenetrationDepthIntoWorld(const EntityId entity, int axis, int direction)
{
    const TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
    const Model* entityModel = m_ecs.get<MeshComponent>(entity).model;
    Vec3 minVertex(entityModel->boundingBoxVertices);
    Vec3 maxVertex(entityModel->boundingBoxVertices + 15);
    minVertex = minVertex * transform.scale + transform.subBlockCoords;
    maxVertex = maxVertex * transform.scale + transform.subBlockCoords;
    IVec3 minBlock = IVec3(minVertex) + transform.blockCoords;
    IVec3 maxBlock = IVec3(maxVertex) + transform.blockCoords;

    float penetrationDepth = 0.0f;
    IVec3 block;
    std::array<int, 2> p{ (axis + 1) % 3, (axis + 2) % 3 };  // perpendicular axes
    block[axis] = direction == 1 ? maxBlock[axis] : minBlock[axis];
    int i = 0;
    for (block[p[0]] = minBlock[p[0]]; i < 1 + minBlock[p[0]] != maxBlock[p[0]];
        block[p[0]] = maxBlock[p[0]])
    {
        int j = 0;
        for (block[p[1]] = minBlock[p[1]]; j < 1 + minBlock[p[1]] != maxBlock[p[1]];
            block[p[1]] = maxBlock[p[1]])
        {
            LOG(std::to_string(block.x) + ", " + std::to_string(block.y) + ", " + std::to_string(block.z));
            bool collidable =
                m_resourcePack.getBlockData(m_chunkManager.getBlock(block)).collidable;
            if (collidable)
            {
                float depth;
                if (block[axis] == minBlock[axis])
                {
                    if (direction == -1)
                        depth = std::ceil(minVertex[axis]) - minVertex[axis];
                    else
                        depth = minVertex[axis] - std::floor(minVertex[axis]);
                }
                if (block[axis] == maxBlock[axis])
                {
                    if (direction == -1)
                        depth = std::ceil(maxVertex[axis]) - maxVertex[axis];
                    else
                        depth = maxVertex[axis] - std::floor(maxVertex[axis]);
                }
                penetrationDepth = std::max(penetrationDepth, depth);
            }
            j++;
        }
        i++;
    }

    return penetrationDepth;
}

void PhysicsEngine::extrapolateTransforms(const float DT)
{
    for (EntityId entity : ECSView<TransformComponent, PhysicsComponent>(m_ecs))
    {
        TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
        PhysicsComponent& physics = m_ecs.get<PhysicsComponent>(entity);
        IVec3 oldBlockCoords = transform.blockCoords;
        Vec3 oldSubBlockCoords = transform.subBlockCoords;
        Vec3 oldRotation = transform.rotation;
        Vec3 oldVelocity = physics.velocity;

        stepPhysics(entity, DT);
        transform.subBlockCoords.x += transform.blockCoords.x - oldBlockCoords.x;
        transform.subBlockCoords.y += transform.blockCoords.y - oldBlockCoords.y;
        transform.subBlockCoords.z += transform.blockCoords.z - oldBlockCoords.z;
        transform.updateTransformMatrix();

        transform.blockCoords = oldBlockCoords;
        transform.subBlockCoords = oldSubBlockCoords;
        transform.rotation = oldRotation;
        physics.velocity = oldVelocity;
    }
}

}  // namespace lonelycube
