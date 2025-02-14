/*
  Lonely Cube, a voxel game
  Copyright (C) g 2024-2025 Bertie Cartwright

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

#include "glm/glm.hpp"
#include "stb_image.h"

#include "client/graphics/vulkan/vulkanEngine.h"
#include <vulkan/vulkan_core.h>

namespace lonelycube::client {

struct SkyPushConstants
{
    glm::vec3 sunDir;
    float brightness;
    glm::mat4 inverseViewProjection;
    glm::vec3 sunGlowColour;
    float sunGlowAmount;
};

class Renderer
{
public:
    SkyPushConstants skyRenderInfo;

    Renderer();
    ~Renderer();
    void beginRenderingFrame();
    void drawSky();
    void drawBlocks(const GPUMeshBuffers& mesh);
    void submitFrame();

    inline VulkanEngine& getVulkanEngine()
    {
        return m_vulkanEngine;
    }

private:
    VulkanEngine m_vulkanEngine;

    VkPipelineLayout m_skyPipelineLayout;
    VkPipeline m_skyPipeline;
    AllocatedImage m_skyImage;
    VkDescriptorSetLayout m_skyImageDescriptorLayout;
    VkDescriptorSet m_skyImageDescriptors;
    VkDescriptorSetLayout m_worldTexturesDescriptorLayout;
    VkDescriptorSet m_worldTexturesDescriptors;

    VkPipelineLayout m_blockPipelineLayout;
    VkPipeline m_blockPipeline;

    AllocatedImage m_worldTextures;
    VkSampler m_worldTexturesSampler;

    void createPipelines();
    void cleanupPipelines();
    void initDescriptors();
    void loadTextures();
    void cleanupTextures();

    void createSkyImage();
    void createSkyPipeline();
    void cleanupSkyPipeline();

    void createBlockPipeline();
    void cleanupBlockPipeline();
};

}  // namespace lonelycube::client
