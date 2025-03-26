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

#include "glm/fwd.hpp"
#include "glm/glm.hpp"

#include "client/graphics/vulkan/descriptors.h"
#include "client/graphics/vulkan/vulkanEngine.h"

namespace lonelycube::client {

struct BloomMip
{
    AllocatedImage image;
    VkDescriptorSet descriptors;
};

class Bloom {
public:
private:
    VulkanEngine& m_vulkanEngine;

    std::vector<BloomMip> m_mipChain;
    AllocatedImage m_srcImage;
    VkDescriptorSetLayout m_descriptorSetLayout;

    void createMips(glm::ivec2 srcTextureSize);
    void deleteMips();
    void renderDownsamples();
    void renderUpsamples(float filterRadius);
public:
    Bloom(VulkanEngine& vulkanEngine);
    void init(DescriptorAllocatorGrowable& descriptorAllocator, AllocatedImage srcImage,
        VkSampler sampler
    );
    void cleanup();
    void render(float filterRadius, float strength);
    AllocatedImage getSmallestMip() {
        return m_mipChain[m_mipChain.size() - 1];
    }
};

}  // namespace lonelycube::client
