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

#include "client/gui/font.h"

#include "glm/gtc/matrix_transform.hpp"

#include "stb_image.h"

namespace lonelycube::client {

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
    calculateCharWidths(textureFilePath);
    resize(windowDimensions);
}

void Font::resize(const uint32_t* windowDimensions)
{
    glm::mat4 projectionMatrix = glm::ortho(
        0.0f, (float)windowDimensions[0], 0.0f, (float)windowDimensions[1], 0.0f, 1.0f
    );
    m_shader.bind();
    m_shader.setUniformMat4f("u_MVP", projectionMatrix);
}

void Font::queue(const std::string& text, glm::ivec2 position, int size,
    const glm::vec3& colour)
{
    const int spacing = std::max(size, 1);

    int verticesSize = m_vertices.size();
    int indicesSize = m_indices.size();
    int numVertices = verticesSize / 28;
    m_vertices.resize(m_vertices.size() + 28 * text.length());
    m_indices.resize(m_indices.size() + 6 * text.length());

    for (char c : text)
    {
        int row = 7 - c / 16;
        int col = c % 16;

        // Screen coordinates
        int charWidth = size * m_charWidths[c - 32];
        m_vertices[verticesSize] = position.x;
        m_vertices[verticesSize + 1] = position.y;
        m_vertices[verticesSize + 7] = position.x;
        m_vertices[verticesSize + 8] = position.y + m_maxCharSize[1] * size;
        m_vertices[verticesSize + 14] = position.x + charWidth;
        m_vertices[verticesSize + 15] = position.y + m_maxCharSize[1] * size;
        m_vertices[verticesSize + 21] = position.x + charWidth;
        m_vertices[verticesSize + 22] = position.y;

        // Texture coordinates
        float charTextureWidth = static_cast<float>(m_charWidths[c - 32]) / m_maxCharSize[0] / 16;
        m_vertices[verticesSize + 2] = static_cast<float>(col) / 16;
        m_vertices[verticesSize + 3] = static_cast<float>(row) / 6;
        m_vertices[verticesSize + 9] = static_cast<float>(col) / 16;
        m_vertices[verticesSize + 10] = static_cast<float>(row) / 6 + 1.0f / 6;
        m_vertices[verticesSize + 16] = static_cast<float>(col) / 16 + charTextureWidth;
        m_vertices[verticesSize + 17] = static_cast<float>(row) / 6 + 1.0f / 6;
        m_vertices[verticesSize + 23] = static_cast<float>(col) / 16 + charTextureWidth;
        m_vertices[verticesSize + 24] = static_cast<float>(row) / 6;

        // Colour data
        for (int i = 0; i < 3; i++)
        {
            m_vertices[verticesSize + 4 + i] = colour[i];
            m_vertices[verticesSize + 11 + i] = colour[i];
            m_vertices[verticesSize + 18 + i] = colour[i];
            m_vertices[verticesSize + 25 + i] = colour[i];
        }

        // Index data
        m_indices[indicesSize] = numVertices;
        m_indices[indicesSize + 1] = numVertices + 3;
        m_indices[indicesSize + 2] = numVertices + 1;
        m_indices[indicesSize + 3] = numVertices + 1;
        m_indices[indicesSize + 4] = numVertices + 3;
        m_indices[indicesSize + 5] = numVertices + 2;

        verticesSize += 28;
        indicesSize += 6;
        numVertices += 4;
        position.x += charWidth + spacing;
    }
}

void Font::draw(const GlRenderer& renderer)
{
    if (m_vertices.empty())
        return;

    m_vertexBuffer.update(&m_vertices.front(), m_vertices.size() * sizeof(float));
    m_indexBuffer.update(&m_indices.front(), m_indices.size());
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
    m_maxCharSize[0] = textureSize[0] / 16;
    m_maxCharSize[1] = textureSize[1] / 6;

    char character = 32;
    for (int row = 0; row < 6; row++)
    {
        for (int col = 0; col < 16; col++)
        {
            int width = 0;
            for (int x = col * m_maxCharSize[0]; x < (col + 1) * m_maxCharSize[0]; x++)
            {
                int y = row * m_maxCharSize[1];
                while (
                    y < (row + 1) * m_maxCharSize[1]
                    && !pixels[
                        y * textureSize[0] * NUM_CHANNELS + x * NUM_CHANNELS + NUM_CHANNELS - 1
                    ]
                )
                    y++;
                if (
                    y < (row + 1) * m_maxCharSize[1]
                    && pixels[
                        y * textureSize[0] * NUM_CHANNELS + x * NUM_CHANNELS + NUM_CHANNELS - 1
                    ]
                )
                    width = x + 1 - col * m_maxCharSize[0];
            }

            m_charWidths[character++ - 32] = width;
        }
    }
    m_charWidths[0] = m_maxCharSize[0] / 2;

    stbi_image_free(pixels);
}

}  // namespace lonelycube::client
