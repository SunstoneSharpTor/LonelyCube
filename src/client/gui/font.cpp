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

#include "client/graphics/vulkan/pipelines.h"
#include "client/graphics/vulkan/shaders.h"
#include "client/graphics/vulkan/vulkanEngine.h"
#include "core/log.h"

namespace lonelycube::client {

const std::string Font::s_textureFilePath = "res/resourcePack/gui/font.png";

Font::Font(VulkanEngine& vulkanEngine) : m_vulkanEngine(vulkanEngine) {}

void Font::init(
    DescriptorAllocatorGrowable& descriptorAllocator, VkPipelineLayout uiPipelineLayout,
    VkDescriptorSetLayout uiImageDescriptorLayout, glm::ivec2 windowDimensions
) {
    m_pipelineLayout = uiPipelineLayout;

    m_vertexBuffers.reserve(VulkanEngine::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < VulkanEngine::MAX_FRAMES_IN_FLIGHT; i++)
        m_vertexBuffers.push_back(m_vulkanEngine.allocateDynamicBuffer(65536));

    int size[2];
    int channels;
    uint8_t* buffer = stbi_load(s_textureFilePath.c_str(), &size[0], &size[1], &channels, 4);
    VkExtent3D extent { static_cast<uint32_t>(size[0]), static_cast<uint32_t>(size[1]), 1 };
    m_fontImage = m_vulkanEngine.createImage(
        buffer, extent, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, 5
    );
    calculateCharWidths(s_textureFilePath);
    stbi_image_free(buffer);
    createDescriptors(descriptorAllocator, uiImageDescriptorLayout);
    createPipeline();

    resize(windowDimensions);
}

void Font::cleanup()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_pipeline, nullptr);
    m_vulkanEngine.destroyImage(m_fontImage);
    for (GPUDynamicBuffer buffer : m_vertexBuffers)
        m_vulkanEngine.destroyHostVisibleAndDeviceLocalBuffer(buffer.buffer);
}

void Font::resize(const glm::ivec2 windowDimensions)
{
    m_pushConstants.mvp = glm::ortho(
        0.0f, static_cast<float>(windowDimensions.x), 0.0f, static_cast<float>(windowDimensions.y),
        0.0f, 1.0f
    );
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

void Font::createDescriptors(
    DescriptorAllocatorGrowable& descriptorAllocator, VkDescriptorSetLayout uiImageDescriptorLayout
) {
    DescriptorWriter writer;

    m_imageDescriptors = descriptorAllocator.allocate(
        m_vulkanEngine.getDevice(), uiImageDescriptorLayout 
    );

    writer.writeImage(
        0, m_fontImage.imageView, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    writer.updateSet(m_vulkanEngine.getDevice(), m_imageDescriptors);
}

void Font::createPipeline()
{
    VkShaderModule vertexShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/font.vert.spv", vertexShader)
    ) {
        LOG("Failed to find shader \"res/shaders/font.vert.spv\"");
    }
    VkShaderModule fragmentShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/font.frag.spv", fragmentShader)
    ) {
        LOG("Failed to find shader \"res/shaders/font.frag.spv\"");
    }

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.pipelineLayout = m_pipelineLayout;
    pipelineBuilder.setShaders(vertexShader, fragmentShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.disableBlending();
    pipelineBuilder.disableDepthTest();
    pipelineBuilder.setColourAttachmentFormat(m_vulkanEngine.getSwapchainImageFormat());
    pipelineBuilder.setDepthAttachmentFormat(VK_FORMAT_UNDEFINED);

    m_pipeline = pipelineBuilder.buildPipeline(m_vulkanEngine.getDevice());

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), vertexShader, nullptr);
    vkDestroyShaderModule(m_vulkanEngine.getDevice(), fragmentShader, nullptr);
}

void Font::queue(const std::string& text, glm::ivec2 position, int size,
    const glm::vec3& colour)
{
    const int spacing = std::max(size, 1);

    float* vertices = static_cast<float*>(
        m_vertexBuffers[m_vulkanEngine.getFrameDataIndex()].buffer.mappedData
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
        vertices[m_vertexBufferSize + 3] = 1.0f - static_cast<float>(row) / 6 - 1.0f / 6;
        vertices[m_vertexBufferSize + 9] = static_cast<float>(col) / 16;
        vertices[m_vertexBufferSize + 10] = 1.0f - static_cast<float>(row) / 6;
        vertices[m_vertexBufferSize + 16] = static_cast<float>(col) / 16 + charTextureWidth;
        vertices[m_vertexBufferSize + 17] = 1.0f - static_cast<float>(row) / 6;
        vertices[m_vertexBufferSize + 23] = static_cast<float>(col) / 16 + charTextureWidth;
        vertices[m_vertexBufferSize + 24] = 1.0f - static_cast<float>(row) / 6 - 1.0f / 6;

        // Colour data
        for (int i = 0; i < 3; i++)
        {
            vertices[m_vertexBufferSize + 4 + i] = colour[i];
            vertices[m_vertexBufferSize + 11 + i] = colour[i];
            vertices[m_vertexBufferSize + 18 + i] = colour[i];
            vertices[m_vertexBufferSize + 25 + i] = colour[i];
        }

        m_vertexBufferSize += 28;
        position.x += charWidth + spacing;
    }
}

void Font::uploadMesh()
{
    if (m_vertexBufferSize == 0)
        return;

    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    m_vulkanEngine.updateDynamicBuffer(
        command, m_vertexBuffers[m_vulkanEngine.getFrameDataIndex()],
        m_vertexBufferSize * sizeof(float)
    );
}

void Font::draw()
{
    if (m_vertexBufferSize == 0)
        return;

    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1, &m_imageDescriptors,
        0, nullptr
    );

    m_pushConstants.vertices = m_vertexBuffers[m_vulkanEngine.getFrameDataIndex()].bufferAddress;
    vkCmdPushConstants(
        command, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(FontPushConstants),
        &m_pushConstants
    );

    vkCmdDraw(command, m_vertexBufferSize / 28 * 6, 1, 0, 0);
    m_vertexBufferSize = 0;
}

}  // namespace lonelycube::client
