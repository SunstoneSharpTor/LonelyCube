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

#include "client/graphics/entityMeshManager.h"

#include "core/constants.h"
#include "core/entities/components/itemComponent.h"
#include "core/entities/components/meshComponent.h"
#include "core/entities/components/transformComponent.h"
#include "core/entities/ECSView.h"

namespace lonelycube {

EntityMeshManager::EntityMeshManager(ServerWorld<true>& serverWorld) :
    m_ecs(serverWorld.getEntityManager().getECS()), m_serverWorld(serverWorld), numIndices(0) {}

void EntityMeshManager::createBatch(
    const IVec3 playerBlockCoords, float* vertexBuffer, uint32_t* indexBuffer,
    const float timeSinceLastTick
) {
    numIndices = 0;
    sizeOfVertices = 0;
    std::lock_guard<std::mutex> lock(m_ecs.mutex);
    for (EntityId entity : ECSView<MeshComponent>(m_ecs))
    {
        const Model* model = m_ecs.get<MeshComponent>(entity).model;
        const uint16_t* textureIndices = m_ecs.get<MeshComponent>(entity).faceTextureIndices;
        const TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
        float entitySkyLight = interpolateSkyLight(transform.blockCoords, transform.subBlockCoords);
        float entityBlockLight = interpolateBlockLight(transform.blockCoords,
                                                       transform.subBlockCoords);
        glm::vec4 offset = glm::vec4(0.0f);
        if (m_ecs.entityHasComponent<ItemComponent>(entity))
        {
            float timer = m_ecs.get<ItemComponent>(entity).timer -
                timeSinceLastTick * constants::TICKS_PER_SECOND;
            offset[1] = std::sin(timer * 0.15f) * 0.06125f + 0.06125f;
        }

        sizeOfVertices += 7;
        for (int faceNum = 0; faceNum < model->faces.size(); faceNum++)
        {
            float texCoords[8];
            ResourcePack::getTextureCoordinates(
                texCoords, model->faces[faceNum].UVcoords, textureIndices[faceNum]
            );
            for (int vertexNum = 0; vertexNum < 4; vertexNum++)
            {
                // Vertex coordinates
                glm::vec4 vertexSubBlock;
                for (int element = 0; element < 3; element++)
                    vertexSubBlock[element] = model->faces[faceNum].coords[vertexNum * 3 + element];
                vertexSubBlock[3] = 1.0f;
                vertexSubBlock = transform.subBlockTransform * vertexSubBlock;
                vertexSubBlock += offset;
                vertexBuffer[sizeOfVertices] = vertexSubBlock.x + (transform.blockCoords.x -
                    playerBlockCoords.x);
                vertexBuffer[sizeOfVertices + 1] = vertexSubBlock.y + (transform.blockCoords.y -
                    playerBlockCoords.y);
                vertexBuffer[sizeOfVertices + 2] = vertexSubBlock.z + (transform.blockCoords.z -
                    playerBlockCoords.z);
                // UV coordinates
                vertexBuffer[sizeOfVertices + 3] = texCoords[vertexNum * 2];
                vertexBuffer[sizeOfVertices + 4] = texCoords[vertexNum * 2 + 1];
                // Sky light
                vertexBuffer[sizeOfVertices + 5] = entitySkyLight;
                // Block light
                vertexBuffer[sizeOfVertices + 6] = entityBlockLight;
                sizeOfVertices += 7;
            }

            //index buffer
            int trueNumVertices = sizeOfVertices / 7;
            indexBuffer[numIndices] = trueNumVertices - 4;
            indexBuffer[numIndices + 1] = trueNumVertices - 3;
            indexBuffer[numIndices + 2] = trueNumVertices - 2;
            indexBuffer[numIndices + 4] = trueNumVertices - 2;
            indexBuffer[numIndices + 5] = trueNumVertices - 1;
            indexBuffer[numIndices + 3] = trueNumVertices - 4;
            numIndices += 6;
        }
    }
}

float EntityMeshManager::interpolateSkyLight(const IVec3& blockCoords, const Vec3& subBlockCoords)
{
    return (float)m_serverWorld.chunkManager.getSkyLight(blockCoords) / constants::skyLightMaxValue;
}

float EntityMeshManager::interpolateBlockLight(const IVec3& blockCoords, const Vec3& subBlockCoords)
{
    return (float)m_serverWorld.chunkManager.getBlockLight(blockCoords) / constants::blockLightMaxValue;
}

}  // namespace lonelycube
