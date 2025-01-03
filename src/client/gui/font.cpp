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

#include "client/graphics/stb_image.h"

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
    calculateCharacterWidths(textureFilePath);
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

void Font::calculateCharWidths(const std::string& textureFilePath)
{
    int textureSize[2];
    const int NUM_CHANNELS = 4;
    stbi_set_flip_vertically_on_load(0);
    uint8_t* pixels = stbi_load(
        textureFilePath.c_str(), &textureSize[0], &textureSize[1], 0, NUM_CHANNELS
    );
    const int maxCharWidth = textureSize[0] / 16;
    const int maxCharHeight = textureSize[1] / 6;

    char character = 32;
    for (int col = 0; col < 6; col++)
    {
        for (int row = 0; row < 16; row++)
        {
            int width = 0;
            for (int x = row * maxCharWidth; x < (row + 1) * maxCharWidth; x++)
            {
                int y = col * maxCharHeight;
                while (
                    y < (col + 1) * maxCharHeight
                    && !pixels[
                        y * textureSize[0] * NUM_CHANNELS + x * NUM_CHANNELS + NUM_CHANNELS - 1
                    ]
                )
                    y++;
                if (
                    y < (col + 1) * maxCharHeight
                    && pixels[
                        y * textureSize[0] * NUM_CHANNELS + x * NUM_CHANNELS + NUM_CHANNELS - 1
                    ]
                )
                    width = x + 1 - row * maxCharWidth;
            }

            m_charWidths[character++] = width;
        }
    }
}

}  // namespace client
