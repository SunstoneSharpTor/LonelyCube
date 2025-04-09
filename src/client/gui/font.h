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

namespace lonelycube::client {

struct FontPushConstants
{
    glm::mat4 mvp;
    VkDeviceAddress vertices;
};

class Font
{
public:
    Font(VulkanEngine& vulkanEngine);
    void init(
        DescriptorAllocatorGrowable& descriptorAllocator,
        VkDescriptorSetLayout uiSamplerDescriptorLayout,
        VkDescriptorSetLayout uiImageDescriptorLayout, VkSampler uiSampler,
        glm::ivec2 windowDimensions
    );
    void cleanup();
    void queue(const std::string& text, glm::ivec2 position, int size, const glm::vec3& colour);
    void draw();
    void resize(const glm::ivec2 windowDimensions);

private:
    static const std::string s_textureFilePath;

    std::array<int, 96> m_charWidths;
    int m_maxCharSize[2];

    VulkanEngine& m_vulkanEngine;

    std::vector<GPUDynamicBuffer> m_vertexBuffers;
    int m_numVertices;
    int m_vertexBufferSize;

    VkSampler m_sampler;
    AllocatedImage m_fontImage;
    VkDescriptorSet m_imageDescriptors;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
    FontPushConstants m_pushConstants;

    void createDescriptors(
        DescriptorAllocatorGrowable& descriptorAllocator,
        VkDescriptorSetLayout uiSamplerDescriptorLayout,
        VkDescriptorSetLayout uiImageDescriptorLayout
    );
    void createPipeline(
        VkDescriptorSetLayout uiSamplerDescriptorLayout,
        VkDescriptorSetLayout uiImageDescriptorLayout
    );
    void calculateCharWidths(const std::string& textureFilePath);
};

}  // namespace lonelycube::client
