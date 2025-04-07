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
    ChunkManager& chunkManager, ECS& ecs, const ResourcePack& resourcePack
) : m_chunkManager(chunkManager), m_ecs(ecs), m_resourcePack(resourcePack) {}

void PhysicsEngine::stepPhysics(const EntityId entity, const float DT
) {
    TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
    PhysicsComponent& physics = m_ecs.get<PhysicsComponent>(entity);
    transform.rotation += physics.angularVelocity * DT;
    if (entityCollidingWithWorld(entity))
    {
        float minPenetrationDepth = 100000.0f;
        int axisOfLeastPenetration = 1;
        int directionToResolve = 1;
        for (int axis = 0; axis < 3; axis++)
        {
            for (int direction = -1; direction <= 1; direction += 2)
            {
                IVec3 neighbouringBlock = transform.blockCoords;
                neighbouringBlock[axis] -= direction;
                if (!m_resourcePack.getBlockData(m_chunkManager.getBlock(
                        neighbouringBlock)).collidable
                ) {
                    float penetrationDepth = findPenetrationDepthIntoWorld(
                        entity, axis, direction * 0.001f
                    );
                    if (penetrationDepth < minPenetrationDepth && penetrationDepth != 0.0f)
                    {
                        minPenetrationDepth = penetrationDepth;
                        axisOfLeastPenetration = axis;
                        directionToResolve = -direction;
                    }
                }
            }
        }
        physics.velocity = Vec3(0.0f, 0.0f, 0.0f);
        physics.velocity[axisOfLeastPenetration] += directionToResolve * 50.0f /
            constants::TICKS_PER_SECOND;
        transform.subBlockCoords[axisOfLeastPenetration] +=
            physics.velocity[axisOfLeastPenetration] * DT;

        return;
    }

    physics.velocity.y -= 20.0f * DT;
    physics.velocity *= 0.98f;
    for (int axis = 0; axis < 3; axis++)
    {
        transform.subBlockCoords[axis] += physics.velocity[axis] * DT;
        if (entityCollidingWithWorld(entity))
        {
            transform.subBlockCoords[axis] += (findPenetrationDepthIntoWorld(
                entity, axis, physics.velocity[axis] * DT
            ) + 0.0001f) * (physics.velocity[axis] > 0 ? -1 : 1);

            physics.velocity[axis] = 0.0f;  // -0.2f * physics.velocity[axis];
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
    std::lock_guard<std::mutex> lock2(m_chunkManager.mutex);
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

float PhysicsEngine::findPenetrationDepthIntoWorld(
    const EntityId entity, const int axis, const float displacementAlongAxis
) {
    const TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
    const Model* entityModel = m_ecs.get<MeshComponent>(entity).model;
    const int direction = displacementAlongAxis > 0 ? 1 : -1;
    Vec3 minVertex(entityModel->boundingBoxVertices);
    Vec3 maxVertex(entityModel->boundingBoxVertices + 15);
    minVertex = minVertex * transform.scale + transform.subBlockCoords;
    maxVertex = maxVertex * transform.scale + transform.subBlockCoords;
    IVec3 minBlock = IVec3(minVertex) + transform.blockCoords;
    IVec3 maxBlock = IVec3(maxVertex) + transform.blockCoords;

    std::array<int, 2> p{ (axis + 1) % 3, (axis + 2) % 3 };  // perpendicular axes
    int startBlock = displacementAlongAxis > 0 ?
        static_cast<int>(maxVertex[axis] - displacementAlongAxis) + transform.blockCoords[axis] :
        static_cast<int>(minVertex[axis] - displacementAlongAxis) + transform.blockCoords[axis];
    int endBlock = displacementAlongAxis > 0 ? maxBlock[axis] : minBlock[axis];
    IVec3 block;
    float penetrationDepth = 0.0f;
    for (
        block[axis] = startBlock;
        block[axis] != endBlock + direction && penetrationDepth == 0.0f;
        block[axis] += direction
    ) {
        for (block[p[0]] = minBlock[p[0]]; block[p[0]] <= maxBlock[p[0]]; block[p[0]]++)
        {
            for (block[p[1]] = minBlock[p[1]]; block[p[1]] <= maxBlock[p[1]]; block[p[1]]++)
            {
                if (m_resourcePack.getBlockData(m_chunkManager.getBlock(block)).collidable)
                {
                    float depth;
                    if (displacementAlongAxis > 0)
                        depth = maxVertex[axis] - std::floor(maxVertex[axis]);
                    else
                        depth = std::ceil(minVertex[axis]) - minVertex[axis];
                    depth += std::abs(endBlock - block[axis]);
                    penetrationDepth = std::max(penetrationDepth, depth);
                }
            }
        }
    }

    return penetrationDepth;
}

void PhysicsEngine::extrapolateTransforms(const float DT)
{
    std::lock_guard<std::mutex> lock1(m_ecs.mutex);
    std::lock_guard<std::mutex> lock2(m_chunkManager.mutex);
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
