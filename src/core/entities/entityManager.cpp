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

#include "core/entities/entityManager.h"

#include "core/entities/components/meshComponent.h"
#include "core/entities/components/transformComponent.h"

EntityManager::EntityManager(int maxNumEntities, const ResourcePack& resourcePack)
    : m_ecs(maxNumEntities), m_resourcePack(resourcePack) {}

void EntityManager::addItem(uint8_t blockType, IVec3 blockCoords, Vec3 subBlockCoords)
{
    EntityId entity = m_ecs.newEntity();
    m_ecs.assign<TransformComponent>(
        entity, blockCoords, subBlockCoords, 0.25f, Vec3(1.0f, 0.0f, 0.0f)
    );
    const Model* blockModel = m_resourcePack.getBlockData(blockType).model;
    const uint16_t* textureIndices = m_resourcePack.getBlockData(blockType).faceTextureIndices;
    m_ecs.assign<MeshComponent>(entity, blockModel, textureIndices);
}
