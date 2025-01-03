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

#include "glm/glm.hpp"

#include "client/graphics/renderer.h"
#include "client/graphics/shader.h"
#include "client/graphics/texture.h"

namespace client {

class Font
{
private:
    Shader m_shader;
    Texture m_texture;
    VertexArray m_vertexArray;
    VertexBuffer m_vertexBuffer;
    VertexBufferLayout m_vbl;
    std::vector<float> m_vertices;

public:
    Font(const std::string& textureFilePath, uint32_t* windowDimensions);

    void queue(const std::string& text, const glm::vec2& position, float size);

    void draw(const Renderer& renderer);

    void resize(uint32_t* windowDimensions);
};

}  // namespace client
