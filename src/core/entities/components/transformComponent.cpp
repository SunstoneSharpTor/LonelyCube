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

#include "core/entities/components/transformComponent.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"

TransformComponent::TransformComponent(IVec3 blockCoords, Vec3 subBlockCoords, Vec3 front)
    : blockCoords(blockCoords), subBlockCoords(subBlockCoords), front(front)
{
    updateTransform();
}

void TransformComponent::updateTransform()
{
    glm::vec3 glmSubBlockCoords(-subBlockCoords.x, -subBlockCoords.y, -subBlockCoords.z);
    glm::vec3 glmFront(-front.x, -front.y, -front.z);
    subBlockTransform = glm::lookAt(glmSubBlockCoords, glmSubBlockCoords + glmFront, glm::vec3(0, 1, 0));
}
