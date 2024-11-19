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

#include "client/entities/meshManager.h"
#include "client/graphics/meshBuilder.h"
#include "core/entities/ECSView.h"
#include "core/entities/components/meshComponent.h"
#include "core/entities/components/transformComponent.h"
#include "core/serverWorld.h"
#include "core/utils/iVec3.h"

MeshManager::MeshManager(int maxVertices, int maxIndices, ServerWorld<true>& serverWorld)
    : vertexBuffer(std::make_unique<float[]>(maxVertices)),
    indexBuffer(std::make_unique<int[]>(maxIndices)), numIndices(0), m_serverWorld(serverWorld) {}

void MeshManager::createBatch(ECS& ecs, IVec3 playerBlockCoords)
{
    numIndices = 0;
    int verticesSize = 0;
    for (EntityId entity : ECSView<MeshComponent>(ecs))
    {
        const Model& model = ecs.get<MeshComponent>(entity).model;
        const uint16_t* textureIndices = ecs.get<MeshComponent>(entity).faceTextureIndices;
        const TransformComponent& transform = ecs.get<TransformComponent>(entity);
        for (int faceNum = 0; faceNum < model.numFaces; faceNum++)
        {
            float texCoords[8];
            client::MeshBuilder::getTextureCoordinates(
                texCoords, model.faces[faceNum].UVcoords, textureIndices[faceNum]
            );
            for (int vertexNum = 0; vertexNum < 4; vertexNum++)
            {
                // Vertex coordinates
                glm::vec4 vertex;
                for (int element = 0; element < 3; element++)
                    vertex[element] = model.faces[faceNum].coords[vertexNum * 3 + element];
                vertex[3] = 1.0f;
                vertex = transform.subBlockTransform * vertex;
                vertexBuffer[verticesSize] = vertex.x + (transform.blockCoords.x -
                    playerBlockCoords.x);
                vertexBuffer[verticesSize + 1] = vertex.y + (transform.blockCoords.y -
                    playerBlockCoords.y);
                vertexBuffer[verticesSize + 2] = vertex.z + (transform.blockCoords.z -
                    playerBlockCoords.z);
                // UV coordinates
                vertexBuffer[verticesSize + 3] = texCoords[vertexNum * 2];
                vertexBuffer[verticesSize + 4] = texCoords[vertexNum * 2 + 1];
                // Sky light
                vertexBuffer[verticesSize + 3] = interpolateSkyLight(IVec3(transform.blockCoords.x, transform.blockCoords.y, transform.blockCoords.z), const Vec3 &subBlockPosition);
                // Block light
                vertex
            }
        }
    }
}


float MeshManager::interpolateSkyLight(const IVec3& blockCoords, const Vec3& subBlockCoords)
{
    return (float)m_serverWorld.getSkyLight(blockCoords) / constants::skyLightMaxValue;
}

float MeshManager::interpolateBlockLight(const IVec3& blockCoords, const Vec3& subBlockCoords)
{
    return (float)m_serverWorld.getBlockLight(blockCoords) / constants::blockLightMaxValue;
}
