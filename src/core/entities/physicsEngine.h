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

#include "core/entities/ECSView.h"
#include "core/entities/components/physicsComponent.h"
#include "core/serverWorld.h"

template<bool integrated>
class PhysicsEngine
{
private:
    ServerWorld<integrated>& m_serverWorld;
    ECS& m_ecs;

public:
    PhysicsEngine(ServerWorld<integrated>& serverWorld, ECS& ecs);
    void stepPhysics(float DT);
};


template<bool integrated>
PhysicsEngine<integrated>::PhysicsEngine(ServerWorld<integrated>& serverWorld, ECS& ecs)
    : m_serverWorld(serverWorld), m_ecs(ecs) {}

template<bool integrated>
void PhysicsEngine<integrated>::stepPhysics(float DT)
{
    for (EntityId entity : ECSView<PhysicsComponent>(m_ecs))
    {

    }
}
