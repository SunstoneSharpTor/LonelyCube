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

#include "core/pch.h"

#include "glm/glm.hpp"

#include "client/graphics/vulkan/descriptors.h"
#include "client/graphics/vulkan/vulkanEngine.h"

namespace lonelycube::client {

struct BloomMip
{
    AllocatedImage image;
    glm::ivec2 renderSize;
    VkDescriptorSet downsampleDescriptors;
    VkDescriptorSet upsampleDescriptors;
};

struct DownsamplePushConstants
{
    glm::vec2 srcTexelSize;
    glm::vec2 srcRenderSize;
    glm::vec2 dstTexelSize;
    float strength;
};

struct UpsamplePushConstants
{
    glm::vec2 srcRenderSize;
    glm::vec2 dstTexelSize;
    float filterRadius;
};

class Bloom {
public:
public:
    Bloom(VulkanEngine& vulkanEngine);
    void init(
        DescriptorAllocatorGrowable& descriptorAllocator, AllocatedImage srcImage, VkSampler sampler
    );
    void cleanup();
    void updateSrcImage(DescriptorAllocatorGrowable& descriptorAllocator, AllocatedImage srcImage);
    void render(float filterRadius, float strength);

private:
    VulkanEngine& m_vulkanEngine;

    int m_smallestMipIndex;

    AllocatedImage m_srcImage;
    std::vector<BloomMip> m_mipChain;
    VkDescriptorSetLayout m_samplerDescriptorLayout;
    VkDescriptorSetLayout m_imagesDescriptorLayout;
    VkDescriptorSet m_samplerDescriptorSet;
    VkDescriptorSet m_blitImageDescriptors;

    VkPipelineLayout m_downsamplePipelineLayout;
    VkPipeline m_downsamplePipeline;
    VkPipelineLayout m_upsamplePipelineLayout;
    VkPipeline m_upsamplePipeline;

    void createMips(DescriptorAllocatorGrowable& descriptorAllocator);
    void createPipelines();
    void renderDownsamples(float strength);
    void renderUpsamples(float filterRadius);
};

}  // namespace lonelycube::client
