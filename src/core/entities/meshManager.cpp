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

#include "meshManager.h"
#include "core/entities/ECSView.h"
#include "core/entities/components/meshComponent.h"
#include "core/entities/components/transformComponent.h"
#include "core/utils/iVec3.h"
#include <memory>

MeshManager::MeshManager(int maxVertices, int maxIndices)
    : vertexBuffer(std::make_unique<float[]>(maxVertices)),
    indexBuffer(std::make_unique<int[]>(maxIndices)), numIndices(0) {}

void MeshManager::generateBatch(ECS& ecs, IVec3 playerBlockCoords)
{
    numIndices = 0;
    int verticesSize = 0;
    for (EntityId entity : ECSView<MeshComponent>(ecs))
    {
        const Model& model = ecs.get<MeshComponent>(entity).model;
        const TransformComponent& transform = ecs.get<TransformComponent>(entity);
        for (int faceNum = 0; faceNum < model.numFaces; faceNum++)
        {
            for (int vertexNum = 0; vertexNum < 4; vertexNum++)
            {
                glm::vec3 vertex = 
                for (int element = 0; element < 3; element++)
                {
                    vertexBuffer[verticesSize] = model.faces[faceNum].coords[vertexNum * 3 +
                        element];
                    vertexBuffer[verticesSize] 
                }
            }
        }
    }
}
