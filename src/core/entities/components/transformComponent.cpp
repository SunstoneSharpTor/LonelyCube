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
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"

TransformComponent::TransformComponent(IVec3 blockCoords, Vec3 subBlockCoords, float scale, Vec3
    rotation) : scale(scale), blockCoords(blockCoords), subBlockCoords(subBlockCoords),
    rotation(rotation)
{
    updateTransform();
}

void TransformComponent::updateTransform()
{
    glm::vec3 glmSubBlockCoords(subBlockCoords.x, subBlockCoords.y, subBlockCoords.z);
    subBlockTransform = glm::translate(glm::mat4(1.0f), glmSubBlockCoords);
    subBlockTransform = glm::rotate(subBlockTransform, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    subBlockTransform = glm::rotate(subBlockTransform, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    subBlockTransform = glm::rotate(subBlockTransform, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    subBlockTransform *= glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
}
