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
    : m_shader("res/shaders/fontVertex.txt", "res/shaders/fontFragment.txt"),
    m_texture(textureFilePath), m_vertexBuffer(nullptr, 0, true), m_indexBuffer(nullptr, 0, true)
{
    m_shader.bind();
    m_shader.setUniform1i("u_fontTexture", 0);
    m_vbl.push<float>(2);
    m_vbl.push<float>(2);
    m_vbl.push<float>(3);
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

void Font::queue(const std::string& text, const glm::vec2& position, float size,
    const glm::vec3& colour)
{
    for (char c : text)
    {
        m_vertices.push_back(25.0f);
        m_vertices.push_back(25.0f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(300.0);
        m_vertices.push_back(25.0f);
        m_vertices.push_back(0.7f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(25.0f);
        m_vertices.push_back(300.0f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(0.7f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(0.5f);
        m_vertices.push_back(0.5f);
        m_indices.push_back(0);
        m_indices.push_back(1);
        m_indices.push_back(2);
    }
}

void Font::draw(const Renderer& renderer)
{
    if (m_vertices.empty())
        return;

    m_vertexBuffer.update(&m_vertices.front(), m_vertices.size() * sizeof(float));
    m_indexBuffer.update(&m_indices.front(), m_indices.size() * sizeof(uint32_t));
    m_vertices.clear();
    m_indices.clear();
    m_texture.bind();
    renderer.draw(m_vertexArray, m_indexBuffer, m_shader);
}

}  // namespace client
