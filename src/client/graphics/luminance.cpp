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

#include "client/graphics/vulkan/vulkanEngine.h"
#include "core/pch.h"

#include "client/graphics/vulkan/shaders.h"
#include "client/graphics/vulkan/utils.h"
#include "core/log.h"

namespace lonelycube::client {

Luminance::Luminance(VulkanEngine& vulkanEngine) : m_vulkanEngine(vulkanEngine) {}

void Luminance::init(
    DescriptorAllocatorGrowable& descriptorAllocator, VkImageView srcImageView, VkSampler sampler
) {
    // Set up SSBOs to store luminance values for the pixels
    for (int i = 0; i < 2; i++)
    {
        m_luminanceBuffers[i] = m_vulkanEngine.createBuffer(
            s_luminanceImageResolution * s_luminanceImageResolution * 4 / (i + 1),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
        );
        VkBufferDeviceAddressInfo deviceAddressInfo{};
        deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        deviceAddressInfo.buffer = m_luminanceBuffers[i].buffer;
        m_parallelReduceMeanPushConstants[i].inputNumbersBuffer = vkGetBufferDeviceAddress(
            m_vulkanEngine.getDevice(), &deviceAddressInfo
        );
        m_parallelReduceMeanPushConstants[(i + 1) % 2].outputNumbersBuffer =
            m_parallelReduceMeanPushConstants[i].inputNumbersBuffer;
    }

    m_luminancePushConstants.luminanceImageSize = s_luminanceImageResolution;
    m_luminancePushConstants.luminanceBuffer =
        m_parallelReduceMeanPushConstants[0].inputNumbersBuffer;

    createLuminanceDescriptors(descriptorAllocator, srcImageView, sampler);
    createLuminancePipeline();
    createParallelReduceMeanPipeline();
}

void Luminance::cleanup()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_luminancePipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_luminancePipelineLayout, nullptr);
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_parallelReduceMeanPipeline, nullptr);
    vkDestroyPipelineLayout(
        m_vulkanEngine.getDevice(), m_parallelReduceMeanPipelineLayout, nullptr
    );
    vkDestroyDescriptorSetLayout(
        m_vulkanEngine.getDevice(), m_luminanceDescriptorSetLayout, nullptr
    );
    m_vulkanEngine.destroyBuffer(m_luminanceBuffers[0]);
    m_vulkanEngine.destroyBuffer(m_luminanceBuffers[1]);
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

    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = m_luminancePipelineLayout;
    pipelineCreateInfo.stage = stageInfo;

    VK_CHECK(vkCreateComputePipelines(
        m_vulkanEngine.getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
        &m_luminancePipeline
    ));

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), luminanceShader, nullptr);
}

void Luminance::createParallelReduceMeanPipeline()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(ParallelReduceMeanPushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr,
        &m_parallelReduceMeanPipelineLayout
    ));

    VkShaderModule parallelReduceMeanShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/parallelReduceMean.comp.spv",
        parallelReduceMeanShader))
    {
        LOG("Failed to find shader \"res/shaders/parallelReduceMean.comp.spv\"");
    }

    uint32_t subgroupSize = m_vulkanEngine.getPhysicalDeviceSubgroupProperties().subgroupSize;

    VkSpecializationMapEntry specializationMapEntry{};
    specializationMapEntry.constantID = 0;
    specializationMapEntry.size = sizeof(int);

    VkSpecializationInfo specializationInfo{};
    specializationInfo.dataSize = sizeof(int);
    specializationInfo.mapEntryCount = 1;
    specializationInfo.pMapEntries = &specializationMapEntry;
    specializationInfo.pData = &subgroupSize;

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = parallelReduceMeanShader;
    stageInfo.pName = "main";
    stageInfo.pSpecializationInfo = &specializationInfo;

    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = m_parallelReduceMeanPipelineLayout;
    pipelineCreateInfo.stage = stageInfo;

    VK_CHECK(vkCreateComputePipelines(
        m_vulkanEngine.getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
        &m_parallelReduceMeanPipeline
    ));

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), parallelReduceMeanShader, nullptr);
}

void Luminance::calculate(const glm::vec2 renderAreaFraction)
{
    // Calculate luminance for the frame and store in an a 1D array on the GPU
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, m_luminancePipeline);
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_COMPUTE, m_luminancePipelineLayout,
        0, 1, &m_luminanceDescriptors,
        0, nullptr
    );

    m_luminancePushConstants.renderAreaFraction = renderAreaFraction;
    vkCmdPushConstants(
        command, m_luminancePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
        sizeof(LuminancePushConstants), &m_luminancePushConstants
    );

    vkCmdDispatch(
        command, (s_luminanceImageResolution + 15) / 16, (s_luminanceImageResolution + 15) / 16, 1
    );

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, m_parallelReduceMeanPipeline);

    VkBufferMemoryBarrier bufMemBarrier{};
    bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufMemBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    bufMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.offset = 0;

    // Reduce the array down to a size that is less than the size of a single subgroup
    // using the mean
    uint32_t workGroupSize = m_vulkanEngine.getPhysicalDeviceSubgroupProperties().subgroupSize;
    uint32_t numWorkGroups = s_luminanceImageResolution * s_luminanceImageResolution /
        workGroupSize;
    int inputBufferIndex = 0;
    while (numWorkGroups > 0)
    {
        bufMemBarrier.buffer = m_luminanceBuffers[inputBufferIndex].buffer;
        bufMemBarrier.size = numWorkGroups * workGroupSize;

        vkCmdPipelineBarrier(
            command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
            0, nullptr, 1, &bufMemBarrier, 0, nullptr
        );

        vkCmdPushConstants(
            command, m_luminancePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
            sizeof(ParallelReduceMeanPushConstants),
            &m_parallelReduceMeanPushConstants[inputBufferIndex]
        );

        vkCmdDispatch(command, numWorkGroups, 1, 1);

        inputBufferIndex = (inputBufferIndex + 1) % 2;
        numWorkGroups /= workGroupSize;
    }

    bufMemBarrier.buffer = m_luminanceBuffers[inputBufferIndex].buffer;
    bufMemBarrier.size = workGroupSize;

    vkCmdPipelineBarrier(
        command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
        0, nullptr, 1, &bufMemBarrier, 0, nullptr
    );
}

}  // namespace lonelycube::client
