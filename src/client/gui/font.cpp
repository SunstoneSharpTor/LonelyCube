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

#include "client/gui/font.h"

#include "glm/gtc/matrix_transform.hpp"

namespace client {

Font::Font(const std::string& textureFilePath, uint32_t* windowDimensions)
    : m_shader("res/shaders/crosshairVertex.txt", "res/shaders/crosshairFragment.txt"),
    m_texture(textureFilePath), m_vertexBuffer(nullptr, 0, true)
{
    m_vbl.push<float>(2);
    m_vertexArray.addBuffer(m_vertexBuffer, m_vbl);
    resize(windowDimensions);
}

void Font::resize(uint32_t* windowDimensions)
{
    glm::mat4 projectionMatrix = glm::ortho(
        -(float)windowDimensions[0] / 2,
        (float)windowDimensions[0] / 2,
        -(float)windowDimensions[1] / 2,
        (float)windowDimensions[1] / 2,
        -1.0f, 1.0f
    );
    m_shader.bind();
    m_shader.setUniformMat4f("u_MVP", projectionMatrix);
}

void Font::queue(const std::string& text, const glm::vec2& position, float size)
{
    for (char c : text)
    {
        m_vertices.push_back(25.0f);
        m_vertices.push_back(25.0f);
        m_vertices.push_back(30.0);
        m_vertices.push_back(25.0f);
        m_vertices.push_back(25.0f);
        m_vertices.push_back(30.0f);
    }
}

void Font::draw(const Renderer& renderer)
{
    if (m_vertices.empty())
        return;

    m_vertexBuffer.update(&m_vertices.front(), m_vertices.size() * sizeof(float));
    m_vertices.clear();
    renderer.draw(m_vertexArray, m_vertices.size() / 2, m_shader);

        uint32_t crosshairIndices[18] = { 0, 1,  2,
                                              3,  4,  5,
                                              6,  5,  4,
                                              4,  7,  6,
                                             10,  8,  9,
                                              8, 10, 11 };

        IndexBuffer crosshairIB(crosshairIndices, 6);
    // renderer.draw(m_vertexArray, crosshairIB, m_shader);
}

}  // namespace client
