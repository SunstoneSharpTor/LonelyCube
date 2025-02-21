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

#pragma once

#include "core/pch.h"

#include "glm/glm.hpp"

#include "client/graphics/glRenderer.h"
#include "client/graphics/shader.h"
#include "client/graphics/texture.h"

namespace lonelycube::client {

class Font
{
private:
    Shader m_shader;
    Texture m_texture;
    VertexArray m_vertexArray;
    VertexBuffer m_vertexBuffer;
    IndexBuffer m_indexBuffer;
    VertexBufferLayout m_vbl;
    std::array<int, 96> m_charWidths;
    std::vector<float> m_vertices;
    std::vector<uint32_t> m_indices;
    int m_maxCharSize[2];

    void calculateCharWidths(const std::string& textureFilePath);

public:
    Font(const std::string& textureFilePath, uint32_t* windowDimensions);

    void queue(const std::string& text, glm::ivec2 position, int size, const glm::vec3& colour);

    void draw(const GlRenderer& renderer);

    void resize(const uint32_t* windowDimensions);
};

}  // namespace lonelycube::client
