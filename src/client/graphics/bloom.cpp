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

#include "client/graphics/bloom.h"

#include "core/pch.h"

#include "client/graphics/vulkan/descriptors.h"
#include "client/graphics/vulkan/images.h"
#include "client/graphics/vulkan/shaders.h"
#include "client/graphics/vulkan/utils.h"
#include "core/log.h"

namespace lonelycube::client {

Bloom::Bloom(VulkanEngine& vulkanEngine) : m_vulkanEngine(vulkanEngine) {}

void Bloom::init(
    DescriptorAllocatorGrowable& descriptorAllocator, AllocatedImage srcImage, VkSampler sampler
) {
    m_srcImage = srcImage;

    // Sampler descriptor set
    DescriptorLayoutBuilder builder;
    DescriptorWriter writer;
    builder.addBinding(0, VK_DESCRIPTOR_TYPE_SAMPLER);
    m_samplerDescriptorLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_COMPUTE_BIT
    );
    m_samplerDescriptorSet = descriptorAllocator.allocate(
        m_vulkanEngine.getDevice(), m_samplerDescriptorLayout
    );
    writer.writeImage(0, VK_NULL_HANDLE, sampler, VK_DESCRIPTOR_TYPE_SAMPLER);
    writer.updateSet(m_vulkanEngine.getDevice(), m_samplerDescriptorSet);

    createMips(descriptorAllocator, sampler);
    createPipelines();
}

void Bloom::cleanup()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_downsamplePipeline, nullptr);
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_upsamplePipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_downsamplePipelineLayout, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_upsamplePipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_vulkanEngine.getDevice(), m_samplerDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_vulkanEngine.getDevice(), m_imagesDescriptorLayout, nullptr);
    for (const auto& mip : m_mipChain)
        m_vulkanEngine.destroyImage(mip.image);
}

void Bloom::createMips(DescriptorAllocatorGrowable& descriptorAllocator, VkSampler sampler) {
    DescriptorLayoutBuilder builder;
    DescriptorWriter writer;

    // Image descriptor sets
    builder.clear();
    builder.addBinding(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    builder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    m_imagesDescriptorLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_COMPUTE_BIT
    );

    // Create mips
    VkExtent3D mipSize(m_srcImage.imageExtent.width, m_srcImage.imageExtent.height, 1);
    while (mipSize.width > 1 || mipSize.height > 1)
    {
        mipSize.width = std::max(mipSize.width / 2, 1u);
        mipSize.height = std::max(mipSize.height / 2, 1u);
        BloomMip& currentMip = m_mipChain.emplace_back();

        currentMip.image = m_vulkanEngine.createImage(
            mipSize, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        );
        currentMip.downsampleDescriptors = descriptorAllocator.allocate(
            m_vulkanEngine.getDevice(), m_imagesDescriptorLayout
        );
        currentMip.upsampleDescriptors = descriptorAllocator.allocate(
            m_vulkanEngine.getDevice(), m_imagesDescriptorLayout
        );
    }

    // Write descriptor sets for mips
    VkImageView prevImageView = m_srcImage.imageView;
    for (int i = 0; i < m_mipChain.size(); i++)
    {
        writer.clear();
        writer.writeImage(
            0, prevImageView, VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        writer.writeImage(
            1, m_mipChain[i].image.imageView, VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_IMAGE_LAYOUT_GENERAL
        );
        writer.updateSet(m_vulkanEngine.getDevice(), m_mipChain[i].downsampleDescriptors);

        if (i < m_mipChain.size() - 1)
        {
            writer.clear();
            writer.writeImage(
                0, m_mipChain[i + 1].image.imageView, VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
            writer.writeImage(
                1, m_mipChain[i].image.imageView, VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                VK_IMAGE_LAYOUT_GENERAL
            );
            writer.updateSet(m_vulkanEngine.getDevice(), m_mipChain[i].upsampleDescriptors);
        }

        prevImageView = m_mipChain[i].image.imageView;
    }
}

void Bloom::createPipelines()
{
    std::array<VkDescriptorSetLayout, 2> setLayouts = {
        m_samplerDescriptorLayout, m_imagesDescriptorLayout
    };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = setLayouts.size();
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(DownsamplePushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr, &m_downsamplePipelineLayout
    ));

    pushConstant.size = sizeof(UpsamplePushConstants);

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr, &m_upsamplePipelineLayout
    ));

    VkShaderModule downsampleShader, upsampleShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/bloomDownsample.comp.spv", downsampleShader))
    {
        LOG("Failed to find shader \"res/shaders/bloomDownsample.comp.spv\"");
    }
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/bloomUpsample.comp.spv", upsampleShader))
    {
        LOG("Failed to find shader \"res/shaders/bloomUpsample.comp.spv\"");
    }

    std::array<VkPipelineShaderStageCreateInfo, 2> stageInfos{};
    stageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfos[0].stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfos[0].module = downsampleShader;
    stageInfos[0].pName = "main";

    stageInfos[1] = stageInfos[0];
    stageInfos[1].module = upsampleShader;

    std::array<VkComputePipelineCreateInfo, 2> pipelineCreateInfos{};
    pipelineCreateInfos[0].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfos[0].layout = m_downsamplePipelineLayout;
    pipelineCreateInfos[0].stage = stageInfos[0];

    pipelineCreateInfos[1].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfos[1].layout = m_upsamplePipelineLayout;
    pipelineCreateInfos[1].stage = stageInfos[1];

    std::array<VkPipeline, 2> pipelines;
    VK_CHECK(vkCreateComputePipelines(
        m_vulkanEngine.getDevice(), VK_NULL_HANDLE, pipelineCreateInfos.size(),
        pipelineCreateInfos.data(), nullptr, pipelines.data()
    ));
    m_downsamplePipeline = pipelines[0];
    m_upsamplePipeline = pipelines[1];

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), downsampleShader, nullptr);
    vkDestroyShaderModule(m_vulkanEngine.getDevice(), upsampleShader, nullptr);
}

void Bloom::renderDownsamples(VkExtent2D renderExtent) {
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, m_downsamplePipeline);
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_COMPUTE, m_downsamplePipelineLayout,
        0, 1, &m_samplerDescriptorSet,
        0, nullptr
    );

    glm::vec2 prevMipTexelSize = glm::vec2(
        1.0f / m_srcImage.imageExtent.width, 1.0f / m_srcImage.imageExtent.height
    );
    glm::ivec2 renderSize(m_srcImage.imageExtent.width, m_srcImage.imageExtent.height);
    int mipIndex = 0;
    while (renderSize.x > 1 || renderSize.y > 1)
    {
        renderSize = glm::max(renderSize / 2, glm::ivec2(1));
        const BloomMip& mip = m_mipChain[mipIndex];

        transitionImage(
            command, mip.image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_MEMORY_WRITE_BIT
        );

        vkCmdBindDescriptorSets(
            command, VK_PIPELINE_BIND_POINT_COMPUTE, m_downsamplePipelineLayout,
            1, 1, &mip.downsampleDescriptors,
            0, nullptr
        );

        DownsamplePushConstants pushConstants;
        pushConstants.srcTexelSize = prevMipTexelSize;
        pushConstants.dstTexelSize = glm::vec2(
            1.0f / mip.image.imageExtent.width, 1.0f / mip.image.imageExtent.height);
        prevMipTexelSize = pushConstants.dstTexelSize;

        vkCmdPushConstants(
            command, m_downsamplePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
            sizeof(DownsamplePushConstants), &pushConstants
        );

        vkCmdDispatch(
            command,
            (mip.image.imageExtent.width + 15) / 16, (mip.image.imageExtent.height + 15) / 16, 1
        );

        transitionImage(
            command, mip.image.image, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_MEMORY_READ_BIT
        );

        mipIndex++;
    }
}

void Bloom::renderUpsamples(float filterRadius, VkExtent2D renderExtent) {
    // m_upsampleShader.bind();
    // m_upsampleShader.setUniform1f("filterRadius", filterRadius);
    //
    // for (int i = m_mipChain.size() - 1; i > 0; i--) {
    //     const BloomMip& mip = m_mipChain[i];
    //     const BloomMip& nextMip = m_mipChain[i-1];
    //
    //     glActiveTexture(GL_TEXTURE0);
    //     glBindTexture(GL_TEXTURE_2D, mip.texture);
    //     glBindImageTexture(0, nextMip.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    //     glDispatchCompute((uint32_t)((nextMip.intSize.x + 7) / 8),
    //         (uint32_t)((nextMip.intSize.y + 7) / 8), 1);
    //     // Make sure writing to image has finished before read
    //     glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    // }
}

void Bloom::render(float filterRadius, float strength, VkExtent2D renderExtent) {
    renderDownsamples(renderExtent);
    renderUpsamples(filterRadius, renderExtent);
    //
    // //glClear(GL_COLOR_BUFFER_BIT);  // Use to test the bloom image before it's composited
    //
    // m_blitShader.bind();
    // m_blitShader.setUniform1f("strength", strength);
    //
    // const BloomMip& mip = m_mipChain[0];
    //
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, mip.texture);
    // glBindImageTexture(0, m_srcTexture.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    // glDispatchCompute((uint32_t)((m_srcTexture.intSize.x + 7) / 8),
    //     (uint32_t)((m_srcTexture.intSize.y + 7) / 8), 1);
    // // Make sure writing to image has finished before read
    // glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

}  // namespace lonelycube::client
