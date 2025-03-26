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

#include "client/graphics/vulkan/descriptors.h"
#include "core/pch.h"

#include "core/log.h"
#include <vulkan/vulkan_core.h>

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

    // Image descriptor sets
    builder.clear();
    builder.addBinding(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    builder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    m_imagesDescriptorLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_COMPUTE_BIT
    );

    // Create mips
    VkExtent3D mipSize(srcImage.imageExtent.width, srcImage.imageExtent.height, 1);
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
    VkImageView prevImageView = srcImage.imageView;
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
            writer.updateSet(m_vulkanEngine.getDevice(), m_mipChain[i].downsampleDescriptors);
        }
    }
}

void Bloom::cleanup()
{
    vkDestroyDescriptorSetLayout(m_vulkanEngine.getDevice(), m_samplerDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_vulkanEngine.getDevice(), m_imagesDescriptorLayout, nullptr);
    for (const auto& mip : m_mipChain)
        m_vulkanEngine.destroyImage(mip.image);
}

void Bloom::renderDownsamples() {
    // m_downsampleShader.bind();
    //
    // // Bind srcTexture (HDR color buffer) as initial texture input
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, m_srcTexture.texture);
    //
    // // Progressively downsample through the mip chain
    // for (int i = 0; i < m_mipChain.size(); i++) {
    //     const BloomMip& mip = m_mipChain[i];
    //     glBindImageTexture(0, mip.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    //     glDispatchCompute((uint32_t)((mip.intSize.x + 7) / 8),
    //         (uint32_t)((mip.intSize.y + 7) / 8), 1);
    //     // Make sure writing to image has finished before read
    //     glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    //
    //     // Set current mip as texture input for next iteration
    //     glBindTexture(GL_TEXTURE_2D, mip.texture);
    // }
}

void Bloom::renderUpsamples(float filterRadius) {
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

void Bloom::render(float filterRadius, float strength) {
    // renderDownsamples();
    // renderUpsamples(filterRadius);
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
