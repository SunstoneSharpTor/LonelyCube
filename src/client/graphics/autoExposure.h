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

struct LuminancePushConstants
{
    VkDeviceAddress luminanceBuffer;
    glm::vec2 renderAreaFraction;
    int luminanceImageSize;
};

struct ParallelReduceMeanPushConstants
{
    VkDeviceAddress inputNumbersBuffer;
    VkDeviceAddress outputNumbersBuffer;
};

struct AutoExposurePushConstants
{
    VkDeviceAddress luminanceBuffer;
    VkDeviceAddress exposureBuffer;
    uint32_t numTicks;
};

class AutoExposure
{
public:
    AutoExposure(VulkanEngine& vulkanEngine);
    void init(
        DescriptorAllocatorGrowable& descriptorAllocator, VkImageView srcImageView,
        VkSampler sampler
    );
    void cleanup();
    void calculate(const glm::vec2 renderAreaFraction, double DT);
    void updateImageView(VkImageView imageView, VkSampler sampler);
    inline VkDeviceAddress getExposureBuffer() const
    {
        return m_autoExposurePushConstants.exposureBuffer;
    }

private:
    static constexpr uint32_t s_luminanceImageResolution = 1024;

    double m_time = 0.0;
    uint64_t m_numTicks = 0;

    VulkanEngine& m_vulkanEngine;

    std::array<AllocatedBuffer, 2> m_luminanceBuffers;
    AllocatedBuffer m_exposureBuffer;

    VkDescriptorSetLayout m_luminanceDescriptorSetLayout;
    VkDescriptorSet m_luminanceDescriptors;
    VkPipelineLayout m_luminancePipelineLayout;
    VkPipeline m_luminancePipeline;
    LuminancePushConstants m_luminancePushConstants;

    VkPipelineLayout m_parallelReduceMeanPipelineLayout;
    VkPipeline m_parallelReduceMeanPipeline;
    std::array<ParallelReduceMeanPushConstants, 2> m_parallelReduceMeanPushConstants;

    VkPipelineLayout m_autoExposurePipelineLayout;
    VkPipeline m_autoExposurePipeline;
    AutoExposurePushConstants m_autoExposurePushConstants;

    void createLuminanceDescriptors(
        DescriptorAllocatorGrowable& descriptorAllocator, VkImageView srcImageView,
        VkSampler sampler
    );
    void createLuminancePipeline();
    void createParallelReduceMeanPipeline();
    void createAutoExposurePipeline(uint32_t numLuminanceElements);

    void calculateLuminancePerPixel(const glm::vec2 renderAreaFraction);
    void parallelReduceMeanLuminance();
    void updateExposure(double DT);
};

}  // namespace lonelycube::client
