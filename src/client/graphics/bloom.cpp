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

#include "core/log.h"

namespace lonelycube::client {

Bloom::Bloom(VulkanEngine& vulkanEngine) : m_vulkanEngine(vulkanEngine) {}

void Bloom::init(
    DescriptorAllocatorGrowable& descriptorAllocator, AllocatedImage srcImage, VkSampler sampler
) {
    m_srcImage = srcImage;
    DescriptorLayoutBuilder builder;
    builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    builder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    m_descriptorSetLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_COMPUTE_BIT
    );

    DescriptorWriter writer;
    glm::ivec2 mipSize(srcImage.imageExtent.width, srcImage.imageExtent.height);
    while (mipSize.x > 1 || mipSize.y > 1)
    {
        mipSize /= 2;
        mipSize = glm::max(mipSize, glm::ivec2(1, 1));


        m_drawImageDescriptors = m_globalDescriptorAllocator.allocate(
            m_vulkanEngine.getDevice(), m_drawImageDescriptorLayout
        );

        writer.writeImage(
            0, m_drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
        );
        writer.updateSet(m_vulkanEngine.getDevice(), m_drawImageDescriptors);
    }

    // createMips(glm::ivec2(srcImage.imageExtent.width, srcImage.imageExtent.height));
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
