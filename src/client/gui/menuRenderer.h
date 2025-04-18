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

#include "glm/glm.hpp"

#include "client/graphics/vulkan/vulkanEngine.h"
#include "client/graphics/vulkan/descriptors.h"
#include "client/gui/font.h"
#include "client/gui/menu.h"

namespace lonelycube::client {

struct MenuRendererPushConstants
{
    glm::mat4 mvp;
    VkDeviceAddress vertices;
};

class MenuRenderer
{
public:
    MenuRenderer(VulkanEngine& vulkanEngine, Font& font);
    void init(
        DescriptorAllocatorGrowable& descriptorAllocator, VkPipelineLayout uiPipelineLayout,
        VkDescriptorSetLayout uiImageDescriptorLayout, glm::ivec2 windowDimensions
    );
    void cleanup();
    void queue(const Menu& menu);
    void uploadMesh();
    void draw();
    void resize(const glm::ivec2 windowDimensions);

private:
    static const std::string s_textureFilePath;

    VulkanEngine& m_vulkanEngine;
    Font& m_font;

    glm::vec2 m_windowDimensions;
    std::vector<GPUDynamicBuffer> m_vertexBuffers;
    int m_vertexBufferSize;

    AllocatedImage m_textures;
    VkDescriptorSet m_samplerDescriptors;
    VkDescriptorSet m_imageDescriptors;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
    MenuRendererPushConstants m_pushConstants;

    void createDescriptors(
        DescriptorAllocatorGrowable& descriptorAllocator,
        VkDescriptorSetLayout uiImageDescriptorLayout
    );
    void createPipeline();
};

}  // namespace lonelycube::client
