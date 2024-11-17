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
#include "core/entities/components/meshComponent.h"
#include <memory>

MeshManager::MeshManager(int maxVertices, int maxIndices)
    : vertexBuffer(std::make_unique<float[]>(maxVertices)),
    indexBuffer(std::make_unique<int[]>(maxIndices)),
    m_freeVertices(7 * maxVertices, 0), m_freeIndices(maxIndices, 0), numIndices(0) {}

MeshComponent MeshManager::addMesh(const Model& model)
{
    int vertexBufferIndex, indexBufferIndex;
    // Allocate space for the vertices in the vertex buffer
    int i = 0;
    while (i < m_freeVertices.size() && m_freeVertices[i] < 7 * 4 * model.numFaces)
        i += 2;
    if (i < m_freeVertices.size())
    {
        vertexBufferIndex = m_freeVertices[i + 1];
        m_freeVertices.erase(m_freeVertices.begin() + i, m_freeVertices.begin() + i + 2);
    }
    else
    {
        std::cout << "Entity mesh vertex buffer full\n";
    }
    // Allocate space for the indices in the index buffer
    i = 0;
    while (i < m_freeIndices.size() && m_freeIndices[i] < 7 * 4 * model.numFaces)
        i += 2;
    if (i < m_freeIndices.size())
    {
        indexBufferIndex = m_freeIndices[i + 1];
        m_freeIndices.erase(m_freeIndices.begin() + i, m_freeIndices.begin() + i + 2);
        if (indexBufferIndex == numIndices)
            numIndices += 6 * model.numFaces;
    }
    else
    {
        std::cout << "Entity mesh index buffer full\n";
    }

    // Set the correct values in the index buffer
    for (int faceNum = 0; faceNum < model.numFaces; faceNum++)
    {
        int vertex = vertexBufferIndex / 7 + faceNum * 4;
        int index = indexBufferIndex + faceNum * 6;
        // indexBuffer[index] = indexBu
    }

    MeshComponent mesh(model, vertexBufferIndex, indexBufferIndex);
    return mesh;
}
