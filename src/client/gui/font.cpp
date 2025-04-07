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

#include "glm/ext/matrix_clip_space.hpp"
#include "stb_image.h"

namespace lonelycube::client {

Font::Font(VulkanEngine& vulkanEngine) : m_vulkanEngine(vulkanEngine) {}

void Font::init(const std::string& textureFilePath, glm::ivec2 windowDimensions)
{
    m_vertexBuffers.reserve(VulkanEngine::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < VulkanEngine::MAX_FRAMES_IN_FLIGHT; i++)
        m_vertexBuffers.push_back(m_vulkanEngine.allocateDynamicBuffer(65536));

    int size[2];
    int channels;
    uint8_t* buffer = stbi_load(textureFilePath.c_str(), &size[0], &size[1], &channels, 4);
    VkExtent3D extent { static_cast<uint32_t>(size[0]), static_cast<uint32_t>(size[1]), 1 };
    m_fontImage = m_vulkanEngine.createImage(
        buffer, extent, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, 5
    );
    calculateCharWidths(textureFilePath);
    stbi_image_free(buffer);
    resize(windowDimensions);
}

void Font::resize(const glm::ivec2 windowDimensions)
{
    m_pushConstants.mvp = glm::ortho(
        0.0f, static_cast<float>(windowDimensions.x), 0.0f, static_cast<float>(windowDimensions.y),
        0.0f, 1.0f
    );
}

void Font::queue(const std::string& text, glm::ivec2 position, int size,
    const glm::vec3& colour)
{
    const int spacing = std::max(size, 1);

    float* vertices = static_cast<float*>(
        m_vertexBuffers[ m_vulkanEngine.getFrameDataIndex()].buffer.mappedData
    );

    for (char c : text)
    {
        int row = 7 - c / 16;
        int col = c % 16;

        // Screen coordinates
        int charWidth = size * m_charWidths[c - 32];
        vertices[m_vertexBufferSize] = position.x;
        vertices[m_vertexBufferSize + 1] = position.y;
        vertices[m_vertexBufferSize + 7] = position.x;
        vertices[m_vertexBufferSize + 8] = position.y + m_maxCharSize[1] * size;
        vertices[m_vertexBufferSize + 14] = position.x + charWidth;
        vertices[m_vertexBufferSize + 15] = position.y + m_maxCharSize[1] * size;
        vertices[m_vertexBufferSize + 21] = position.x + charWidth;
        vertices[m_vertexBufferSize + 22] = position.y;

        // Texture coordinates
        float charTextureWidth = static_cast<float>(m_charWidths[c - 32]) / m_maxCharSize[0] / 16;
        vertices[m_vertexBufferSize + 2] = static_cast<float>(col) / 16;
        vertices[m_vertexBufferSize + 3] = static_cast<float>(row) / 6;
        vertices[m_vertexBufferSize + 9] = static_cast<float>(col) / 16;
        vertices[m_vertexBufferSize + 10] = static_cast<float>(row) / 6 + 1.0f / 6;
        vertices[m_vertexBufferSize + 16] = static_cast<float>(col) / 16 + charTextureWidth;
        vertices[m_vertexBufferSize + 17] = static_cast<float>(row) / 6 + 1.0f / 6;
        vertices[m_vertexBufferSize + 23] = static_cast<float>(col) / 16 + charTextureWidth;
        vertices[m_vertexBufferSize + 24] = static_cast<float>(row) / 6;

        // Colour data
        for (int i = 0; i < 3; i++)
        {
            vertices[m_vertexBufferSize + 4 + i] = colour[i];
            vertices[m_vertexBufferSize + 11 + i] = colour[i];
            vertices[m_vertexBufferSize + 18 + i] = colour[i];
            vertices[m_vertexBufferSize + 25 + i] = colour[i];
        }

        m_vertexBufferSize += 28;
        m_numVertices += 4;
        position.x += charWidth + spacing;
    }
}

void Font::draw(const GlRenderer& renderer)
{
    // if (vertices.empty())
    //     return;
    //
    // m_vertexBuffer.update(&vertices.front(), vertices.size() * sizeof(float));
    // m_indexBuffer.update(&m_indices.front(), m_indices.size());
    // vertices.clear();
    // m_indices.clear();
    // m_texture.bind();
    // renderer.draw(m_vertexArray, m_indexBuffer, m_shader);
    m_numVertices = m_vertexBufferSize = 0;
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
