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

#include "client/graphics/autoExposure.h"

#include "core/pch.h"

#include "client/graphics/vulkan/shaders.h"
#include "client/graphics/vulkan/utils.h"
#include "client/graphics/vulkan/vulkanEngine.h"
#include "core/constants.h"
#include "core/log.h"

namespace lonelycube::client {

AutoExposure::AutoExposure(VulkanEngine& vulkanEngine) : m_vulkanEngine(vulkanEngine) {}

void AutoExposure::init(
    DescriptorAllocatorGrowable& descriptorAllocator, VkImageView srcImageView, VkSampler sampler
) {
    // Set up SSBOs to store luminance values for the pixels
    const uint32_t subgroupSize = m_vulkanEngine.getPhysicalDeviceSubgroupProperties().subgroupSize;
    for (int i = 0; i < 2; i++)
    {
        const uint32_t bufferSize = s_luminanceImageResolution * s_luminanceImageResolution * 4 /
            (i == 0 ? 1 : subgroupSize * 2);
        m_luminanceBuffers[i] = m_vulkanEngine.createBuffer(
            bufferSize,
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

    // Set up exposure SSBO
    m_exposureBuffer = m_vulkanEngine.createBuffer(
        4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    );
    AllocatedBuffer staging = m_vulkanEngine.createBuffer(
        4,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT 
    );
    float initialExposure = 0.0f;
    memcpy(staging.info.pMappedData, &initialExposure, 4);

    m_vulkanEngine.immediateSubmit([&](VkCommandBuffer command) {
        VkBufferCopy copy{};
        copy.size = 4;
        vkCmdCopyBuffer(command, staging.buffer, m_exposureBuffer.buffer, 1, &copy);
    });
    m_vulkanEngine.destroyBuffer(staging);

    // Set up auto exposure push constants
    uint32_t numWorkGroups = s_luminanceImageResolution * s_luminanceImageResolution;
    int numParallelReducePasses = 0;
    while (numWorkGroups >= subgroupSize * 2)
    {
        numWorkGroups /= subgroupSize * 2;
        numParallelReducePasses++;
    }

    m_autoExposurePushConstants.luminanceBuffer = m_parallelReduceMeanPushConstants[
        numParallelReducePasses % 2].inputNumbersBuffer;

    VkBufferDeviceAddressInfo deviceAddressInfo{};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer = m_exposureBuffer.buffer;
    m_autoExposurePushConstants.exposureBuffer = vkGetBufferDeviceAddress(
        m_vulkanEngine.getDevice(), &deviceAddressInfo
    );

    createLuminanceDescriptors(descriptorAllocator, srcImageView, sampler);
    createLuminancePipeline();
    createParallelReduceMeanPipeline();
    createAutoExposurePipeline(numWorkGroups);
}

void AutoExposure::cleanup()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_luminancePipeline, nullptr);
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_parallelReduceMeanPipeline, nullptr);
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_autoExposurePipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_luminancePipelineLayout, nullptr);
    vkDestroyPipelineLayout(
        m_vulkanEngine.getDevice(), m_parallelReduceMeanPipelineLayout, nullptr
    );
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_autoExposurePipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(
        m_vulkanEngine.getDevice(), m_luminanceDescriptorSetLayout, nullptr
    );
    m_vulkanEngine.destroyBuffer(m_luminanceBuffers[0]);
    m_vulkanEngine.destroyBuffer(m_luminanceBuffers[1]);
    m_vulkanEngine.destroyBuffer(m_exposureBuffer);
}

void AutoExposure::createLuminanceDescriptors(
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
        0, srcImageView, sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    writer.updateSet(m_vulkanEngine.getDevice(), m_luminanceDescriptors);
}

void AutoExposure::createLuminancePipeline()
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

void AutoExposure::createParallelReduceMeanPipeline()
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

    const uint32_t subgroupSize = m_vulkanEngine.getPhysicalDeviceSubgroupProperties().subgroupSize;

    VkSpecializationMapEntry specializationMapEntry{};
    specializationMapEntry.constantID = 0;
    specializationMapEntry.size = sizeof(uint32_t);

    VkSpecializationInfo specializationInfo{};
    specializationInfo.dataSize = sizeof(uint32_t);
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

void AutoExposure::createAutoExposurePipeline(uint32_t numLuminanceElements)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(AutoExposurePushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr,
        &m_autoExposurePipelineLayout
    ));

    VkShaderModule autoExposureShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/autoExposure.comp.spv",
        autoExposureShader))
    {
        LOG("Failed to find shader \"res/shaders/autoExposure.comp.spv\"");
    }

    VkSpecializationMapEntry specializationMapEntry{};
    specializationMapEntry.constantID = 0;
    specializationMapEntry.size = sizeof(uint32_t);

    VkSpecializationInfo specializationInfo{};
    specializationInfo.dataSize = sizeof(int);
    specializationInfo.mapEntryCount = 1;
    specializationInfo.pMapEntries = &specializationMapEntry;
    specializationInfo.pData = &numLuminanceElements;

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = autoExposureShader;
    stageInfo.pName = "main";
    stageInfo.pSpecializationInfo = &specializationInfo;

    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = m_autoExposurePipelineLayout;
    pipelineCreateInfo.stage = stageInfo;

    VK_CHECK(vkCreateComputePipelines(
        m_vulkanEngine.getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
        &m_autoExposurePipeline
    ));

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), autoExposureShader, nullptr);
}

void AutoExposure::calculateLuminancePerPixel(const glm::vec2 renderAreaFraction)
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    // Calculate luminance for the frame and store in an a 1D array on the GPU
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
}

void AutoExposure::parallelReduceMeanLuminance()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, m_parallelReduceMeanPipeline);

    VkBufferMemoryBarrier bufMemBarrier{};
    bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufMemBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    bufMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.offset = 0;

    // Reduce the array down to a size that is less than the size of that a single subgroup
    // can reduce using the mean (workGroupSize * 2)
    const uint32_t workGroupSize = m_vulkanEngine.getPhysicalDeviceSubgroupProperties().subgroupSize;
    uint32_t numWorkGroups = s_luminanceImageResolution * s_luminanceImageResolution;
    int inputBufferIndex = 0;
    while (numWorkGroups >= workGroupSize * 2)
    {
        numWorkGroups /= workGroupSize * 2;

        bufMemBarrier.buffer = m_luminanceBuffers[inputBufferIndex].buffer;
        bufMemBarrier.size = numWorkGroups * workGroupSize * 2 * 4;

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

        inputBufferIndex = !inputBufferIndex;
    }

    bufMemBarrier.buffer = m_luminanceBuffers[inputBufferIndex].buffer;
    bufMemBarrier.size = workGroupSize;

    vkCmdPipelineBarrier(
        command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
        0, nullptr, 1, &bufMemBarrier, 0, nullptr
    );
}

void AutoExposure::updateExposure(double DT)
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, m_autoExposurePipeline);

    m_time += DT;
    uint64_t newNumTicks = static_cast<uint64_t>(m_time * constants::visualTPS);
    m_autoExposurePushConstants.numTicks = newNumTicks - m_numTicks;
    m_numTicks = newNumTicks;
    vkCmdPushConstants(
        command, m_luminancePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
        sizeof(AutoExposurePushConstants),
        &m_autoExposurePushConstants
    );

    vkCmdDispatch(command, 1, 1, 1);

    VkBufferMemoryBarrier bufMemBarrier{};
    bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufMemBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    bufMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.offset = 0;
    bufMemBarrier.buffer = m_exposureBuffer.buffer;
    bufMemBarrier.size = 4;

    vkCmdPipelineBarrier(
        command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr, 1, &bufMemBarrier, 0, nullptr
    );
}

void AutoExposure::calculate(const glm::vec2 renderAreaFraction, double DT)
{
    calculateLuminancePerPixel(renderAreaFraction);
    parallelReduceMeanLuminance();
    updateExposure(DT);
}

}  // namespace lonelycube::client
