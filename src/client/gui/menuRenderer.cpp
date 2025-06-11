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

#include "client/gui/menuRenderer.h"

#include "client/gui/menu.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/fwd.hpp"
#include "stb_image.h"

#include "client/graphics/vulkan/pipelines.h"
#include "client/graphics/vulkan/shaders.h"
#include "client/graphics/vulkan/vulkanEngine.h"
#include "core/log.h"

namespace lonelycube::client {

const std::string MenuRenderer::s_textureFilePath = "res/resourcePack/gui/menus.png";

MenuRenderer::MenuRenderer(
    VulkanEngine& vulkanEngine, Font& font
) : m_vulkanEngine(vulkanEngine), m_font(font) {}

void MenuRenderer::init(
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
    m_textures = m_vulkanEngine.createImage(
        buffer, extent, 4, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT
    );
    stbi_image_free(buffer);
    createDescriptors(descriptorAllocator, uiImageDescriptorLayout);
    createPipeline();

    resize(windowDimensions);
}

void MenuRenderer::cleanup()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_pipeline, nullptr);
    m_vulkanEngine.destroyImage(m_textures);
    for (GPUDynamicBuffer buffer : m_vertexBuffers)
        m_vulkanEngine.destroyHostVisibleAndDeviceLocalBuffer(buffer.buffer);
}

void MenuRenderer::resize(const glm::ivec2 windowDimensions)
{
    m_windowDimensions = windowDimensions;
    m_pushConstants.mvp = glm::ortho(
        0.0f, m_windowDimensions.x, 0.0f, m_windowDimensions.y, 0.0f, 1.0f
    );
}

void MenuRenderer::createDescriptors(
    DescriptorAllocatorGrowable& descriptorAllocator, VkDescriptorSetLayout uiImageDescriptorLayout
) {
    DescriptorWriter writer;

    m_imageDescriptors = descriptorAllocator.allocate(
        m_vulkanEngine.getDevice(), uiImageDescriptorLayout 
    );

    writer.writeImage(
        0, m_textures.imageView, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    writer.updateSet(m_vulkanEngine.getDevice(), m_imageDescriptors);
}

void MenuRenderer::createPipeline()
{
    VkShaderModule vertexShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/menu.vert.spv", vertexShader)
    ) {
        LOG("Failed to find shader \"res/shaders/menu.vert.spv\"");
    }
    VkShaderModule fragmentShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/menu.frag.spv", fragmentShader)
    ) {
        LOG("Failed to find shader \"res/shaders/menu.frag.spv\"");
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

void MenuRenderer::queue(const Menu& menu)
{
    float* vertices = static_cast<float*>(
        m_vertexBuffers[m_vulkanEngine.getFrameDataIndex()].buffer.mappedData
    );

    for (const auto& element : menu.getElements())
    {
        switch (element.type)
        {
        case Menu::ElementType::Button:
            glm::ivec2 position = element.buttonData.screenAlignment * m_windowDimensions +
                glm::vec2(element.buttonData.pixelOffset * menu.getScale());
            glm::ivec2 size(element.buttonData.width * menu.getScale(), 14.0f * menu.getScale());

            // Screen coordinates
            vertices[m_vertexBufferSize] = position.x;
            vertices[m_vertexBufferSize + 1] = position.y;
            vertices[m_vertexBufferSize + 4] = position.x;
            vertices[m_vertexBufferSize + 5] = position.y + size.y;
            vertices[m_vertexBufferSize + 8] = position.x + size.x;
            vertices[m_vertexBufferSize + 9] = position.y + size.y;
            vertices[m_vertexBufferSize + 12] = position.x + size.x;
            vertices[m_vertexBufferSize + 13] = position.y;

            // Texture coordinates
            if (element.buttonData.mouseOver)
            {
                vertices[m_vertexBufferSize + 2] = 0.0f;
                vertices[m_vertexBufferSize + 3] = 14.0f / 512;
                vertices[m_vertexBufferSize + 6] = 0.0f;
                vertices[m_vertexBufferSize + 7] = 28.0f / 512;
                vertices[m_vertexBufferSize + 10] = 160.0f / 512;
                vertices[m_vertexBufferSize + 11] = 28.0f / 512;
                vertices[m_vertexBufferSize + 14] = 160.0f / 512;
                vertices[m_vertexBufferSize + 15] = 14.0f / 512;
            }
            else
            {
                vertices[m_vertexBufferSize + 2] = 0.0f;
                vertices[m_vertexBufferSize + 3] = 0.0f;
                vertices[m_vertexBufferSize + 6] = 0.0f;
                vertices[m_vertexBufferSize + 7] = 14.0f / 512;
                vertices[m_vertexBufferSize + 10] = 160.0f / 512;
                vertices[m_vertexBufferSize + 11] = 14.0f / 512;
                vertices[m_vertexBufferSize + 14] = 160.0f / 512;
                vertices[m_vertexBufferSize + 15] = 0.0f;
            }

            glm::ivec2 textPos = position + glm::ivec2(
                (element.buttonData.width - m_font.getStringWidth(element.text)) / 2, 4
            ) * menu.getScale();
            m_font.queue(element.text, textPos + glm::ivec2(menu.getScale(), menu.getScale()),
                         menu.getScale(), { 0.1f, 0.1f, 0.1f });
            m_font.queue(element.text, textPos, menu.getScale(), { 1.0f, 1.0f, 1.0f });

            break;
        }

        m_vertexBufferSize += 16;
    }
}

void MenuRenderer::uploadMesh()
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

void MenuRenderer::draw()
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
        command, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MenuRendererPushConstants),
        &m_pushConstants
    );

    vkCmdDraw(command, m_vertexBufferSize / 16 * 6, 1, 0, 0);
    m_vertexBufferSize = 0;
}

}  // namespace lonelycube::client
