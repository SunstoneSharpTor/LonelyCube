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

#include "client/graphics/luminance.h"

#include "core/pch.h"

#include "client/graphics/vulkan/shaders.h"
#include "client/graphics/vulkan/utils.h"
#include "core/log.h"

namespace lonelycube::client {

Luminance::Luminance(VulkanEngine& vulkanEngine) : m_vulkanEngine(vulkanEngine) {}

void Luminance::init(
    DescriptorAllocatorGrowable& descriptorAllocator, VkImageView srcImageView, VkSampler sampler
) {
    m_luminanceBuffer = m_vulkanEngine.createBuffer(
        s_luminanceImageResolution * s_luminanceImageResolution * 4,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    );
    m_luminancePushConstants.luminanceImageSize = s_luminanceImageResolution;
    VkBufferDeviceAddressInfo deviceAddressInfo{};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer = m_luminanceBuffer.buffer;
    m_luminancePushConstants.luminanceBuffer = vkGetBufferDeviceAddress(
        m_vulkanEngine.getDevice(), &deviceAddressInfo
    );

    createLuminanceDescriptors(descriptorAllocator, srcImageView, sampler);
    createLuminancePipeline();
}

void Luminance::cleanup()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_luminancePipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_luminancePipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(
        m_vulkanEngine.getDevice(), m_luminanceDescriptorSetLayout, nullptr
    );
    m_vulkanEngine.destroyBuffer(m_luminanceBuffer);
}

void Luminance::createLuminanceDescriptors(
    DescriptorAllocatorGrowable& descriptorAllocator, VkImageView srcImageView, VkSampler sampler
) {
    DescriptorLayoutBuilder builder;
    DescriptorWriter writer;

    builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    m_luminanceDescriptorSetLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_COMPUTE_BIT
    );

    m_luminanceDescriptors = descriptorAllocator.allocate(
        m_vulkanEngine.getDevice(), m_luminanceDescriptorSetLayout
    );

    writer.writeImage(
        0, srcImageView, sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    );
    writer.updateSet(m_vulkanEngine.getDevice(), m_luminanceDescriptors);
}

void Luminance::createLuminancePipeline()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_luminanceDescriptorSetLayout;

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(LuminancePushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr, &m_luminancePipelineLayout
    ));

    VkShaderModule luminanceShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/luminance.comp.spv", luminanceShader))
    {
        LOG("Failed to find shader \"res/shaders/luminance.comp.spv\"");
    }

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = luminanceShader;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo skyPipelineCreateInfo{};
    skyPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    skyPipelineCreateInfo.layout = m_luminancePipelineLayout;
    skyPipelineCreateInfo.stage = stageInfo;

    VK_CHECK(vkCreateComputePipelines(
        m_vulkanEngine.getDevice(), VK_NULL_HANDLE, 1, &skyPipelineCreateInfo, nullptr,
        &m_luminancePipeline
    ));

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), luminanceShader, nullptr);
}

void Luminance::calculate()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, m_luminancePipeline);
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_COMPUTE, m_luminancePipelineLayout,
        0, 1, &m_luminanceDescriptors,
        0, nullptr
    );

    vkCmdPushConstants(
        command, m_luminancePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
        sizeof(LuminancePushConstants), &m_luminancePushConstants
    );

    vkCmdDispatch(
        command, (s_luminanceImageResolution + 15) / 16, (s_luminanceImageResolution + 15) / 16, 1
    );
}

}  // namespace lonelycube::client
