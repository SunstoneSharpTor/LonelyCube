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

#include "core/pch.h"

#include "core/entities/components/meshComponent.h"
#include "core/entities/components/transformComponent.h"
#include "core/entities/ECS.h"
#include "core/entities/ECSView.h"
#include "core/serverWorld.h"
#include "core/utils/iVec3.h"
#include "core/utils/vec3.h"

template<bool integrated>
class MeshManager
{
private:
    ECS& m_ecs;
    ServerWorld<integrated>& m_serverWorld;

    float interpolateSkyLight(const IVec3& blockPosition, const Vec3& subBlockPosition);
    float interpolateBlockLight(const IVec3& blockPosition, const Vec3& subBlockPosition);

public:
    std::unique_ptr<float[]> vertexBuffer;
    std::unique_ptr<uint32_t[]> indexBuffer;
    uint32_t numIndices;
    uint32_t sizeOfVertices;

    MeshManager(ServerWorld<integrated>& serverWorld, int maxVertices, int maxIndices);
    void createBatch(IVec3 playerBlockCoords);
};


template<bool integrated>
MeshManager<integrated>::MeshManager(ServerWorld<integrated>& serverWorld, int maxVertices, int
    maxIndices)
    : m_ecs(serverWorld.getEntityManager().getECS()), m_serverWorld(serverWorld),
    vertexBuffer(std::make_unique<float[]>(maxVertices)),
    indexBuffer(std::make_unique<uint32_t[]>(maxIndices)), numIndices(0) {}

template<bool integrated>
void MeshManager<integrated>::createBatch(IVec3 playerBlockCoords)
{
    numIndices = 0;
    sizeOfVertices = 0;
    for (EntityId entity : ECSView<MeshComponent>(m_ecs))
    {
        const Model* model = m_ecs.get<MeshComponent>(entity).model;
        const uint16_t* textureIndices = m_ecs.get<MeshComponent>(entity).faceTextureIndices;
        const TransformComponent& transform = m_ecs.get<TransformComponent>(entity);
        for (int faceNum = 0; faceNum < model->numFaces; faceNum++)
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
                IVec3 vertexBlock(transform.blockCoords.x, transform.blockCoords.y, transform.blockCoords.z);
                vertexBuffer[sizeOfVertices + 5] = interpolateSkyLight(vertexBlock, Vec3(0.0f, 0.0f, 0.0f));
                // Block light
                vertexBuffer[sizeOfVertices + 6] = interpolateBlockLight(vertexBlock, Vec3(0.0f, 0.0f, 0.0f));
                sizeOfVertices += 7;
                for (int i = 0; i < 7; i++)
                    std::cout << vertexBuffer[sizeOfVertices - 7 + i] << ", ";
                std::cout << "\n";
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

template<bool integrated>
float MeshManager<integrated>::interpolateSkyLight(const IVec3& blockCoords, const Vec3& subBlockCoords)
{
    return (float)m_serverWorld.getSkyLight(blockCoords) / constants::skyLightMaxValue;
}

template<bool integrated>
float MeshManager<integrated>::interpolateBlockLight(const IVec3& blockCoords, const Vec3& subBlockCoords)
{
    return (float)m_serverWorld.getBlockLight(blockCoords) / constants::blockLightMaxValue;
}
