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

#include "client/graphics/renderer.h"

#include "stb_image.h"

#include "client/graphics/vulkan/images.h"
#include "client/graphics/vulkan/pipelines.h"
#include "client/graphics/vulkan/shaders.h"
#include "client/graphics/vulkan/utils.h"
#include "core/log.h"
#include <vulkan/vulkan_core.h>

namespace lonelycube::client {

Renderer::Renderer()
{
    m_vulkanEngine.init();

    createSkyImage();
    loadTextures();
    createDescriptors();
    createPipelines();
}

Renderer::~Renderer()
{
    vkDeviceWaitIdle(m_vulkanEngine.getDevice());

    cleanupTextures();
    cleanupPipelines();
    cleanupDescriptors();
    cleanupSkyImage();
    m_vulkanEngine.cleanup();
}

void Renderer::createDescriptors()
{
    DescriptorLayoutBuilder builder;
    DescriptorWriter writer;

    // Sky image
    builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    m_skyImageDescriptorLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_COMPUTE_BIT
    );

    m_skyImageDescriptors = m_vulkanEngine.getGlobalDescriptorAllocator().allocate(
        m_vulkanEngine.getDevice(), m_skyImageDescriptorLayout
    );

    writer.writeImage(
        0, m_skyImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
    );
    writer.updateSet(m_vulkanEngine.getDevice(), m_skyImageDescriptors);

    // World textures
    builder.clear();
    builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    builder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    m_worldTexturesDescriptorLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT
    );

    m_worldTexturesDescriptors = m_vulkanEngine.getGlobalDescriptorAllocator().allocate(
        m_vulkanEngine.getDevice(), m_worldTexturesDescriptorLayout
    );

    writer.clear();
    writer.writeImage(
        0, m_worldTextures.imageView, m_worldTexturesSampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    );
    writer.writeImage(
        1, m_skyImage.imageView, m_skyImageSampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    );
    writer.updateSet(m_vulkanEngine.getDevice(), m_worldTexturesDescriptors);
}

void Renderer::cleanupDescriptors()
{
    vkDestroyDescriptorSetLayout(m_vulkanEngine.getDevice(), m_skyImageDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(
        m_vulkanEngine.getDevice(), m_worldTexturesDescriptorLayout, nullptr
    );
}

void Renderer::loadTextures()
{
    int size[2];
    int chanels;
    uint8_t* buffer = stbi_load("res/resourcePack/textures.png", &size[0], &size[1], &chanels, 4);
    VkExtent3D extent { static_cast<uint32_t>(size[0]), static_cast<uint32_t>(size[1]), 1 };
    m_worldTextures = m_vulkanEngine.createImage(
        buffer, extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT
    );
    stbi_image_free(buffer);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    vkCreateSampler(m_vulkanEngine.getDevice(), &samplerInfo, nullptr, &m_worldTexturesSampler);
}

void Renderer::cleanupTextures()
{
    vkDestroySampler(m_vulkanEngine.getDevice(), m_worldTexturesSampler, nullptr);
    vkDestroyImageView(m_vulkanEngine.getDevice(), m_worldTextures.imageView, nullptr);
    vmaDestroyImage(
        m_vulkanEngine.getAllocator(), m_worldTextures.image, m_worldTextures.allocation
    );
}

void Renderer::createSkyImage()
{
    VkExtent3D skyImageExtent{
        m_vulkanEngine.getDrawImageExtent().width, m_vulkanEngine.getDrawImageExtent().height, 1
    };

    VkImageUsageFlags skyImageUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT;

    m_skyImage = m_vulkanEngine.createImage(
        skyImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, skyImageUsages, false
    );

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;

    vkCreateSampler(m_vulkanEngine.getDevice(), &samplerInfo, nullptr, &m_skyImageSampler);
}

void Renderer::cleanupSkyImage()
{
    vkDestroySampler(m_vulkanEngine.getDevice(), m_skyImageSampler, nullptr);
    vkDestroyImageView(m_vulkanEngine.getDevice(), m_skyImage.imageView, nullptr);
    vmaDestroyImage(m_vulkanEngine.getAllocator(), m_skyImage.image, m_skyImage.allocation);
}

void Renderer::createSkyPipeline()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_skyImageDescriptorLayout;

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(SkyPushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr, &m_skyPipelineLayout
    ));

    VkShaderModule computeDrawShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/sky.comp.spv", computeDrawShader))
    {
        LOG("Failed to find shader \"res/shaders/sky.comp.spv\"");
    }

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = computeDrawShader;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.layout = m_skyPipelineLayout;
    computePipelineCreateInfo.stage = stageInfo;

    VK_CHECK(vkCreateComputePipelines(
        m_vulkanEngine.getDevice(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr,
        &m_skyPipeline
    ));

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), computeDrawShader, nullptr);
}

void Renderer::cleanupSkyPipeline()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_skyPipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_skyPipelineLayout, nullptr);
}

void Renderer::createBlockPipeline()
{
    VkPushConstantRange bufferRange{};
    bufferRange.size = sizeof(BlockPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &bufferRange;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_worldTexturesDescriptorLayout;

    VK_CHECK(
        vkCreatePipelineLayout(m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr, &m_blockPipelineLayout)
    );

    VkShaderModule blockVertexShader;
    if (!createShaderModule(m_vulkanEngine.getDevice(), "res/shaders/block.vert.spv", blockVertexShader))
        LOG("Failed to find shader \"res/shaders/block.vert.spv\"");

    VkShaderModule blockFragmentShader;
    if (!createShaderModule(m_vulkanEngine.getDevice(), "res/shaders/block.frag.spv", blockFragmentShader))
        LOG("Failed to find shader \"res/shaders/block.frag.spv\"");

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.pipelineLayout = m_blockPipelineLayout;
    pipelineBuilder.setShaders(blockVertexShader, blockFragmentShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.disableBlending();
    pipelineBuilder.disableDepthTest();
    pipelineBuilder.setColourAttachmentFormat(m_vulkanEngine.getDrawImage().imageFormat);
    pipelineBuilder.setDepthAttachmentFormat(VK_FORMAT_UNDEFINED);

    m_blockPipeline = pipelineBuilder.buildPipeline(m_vulkanEngine.getDevice());

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), blockVertexShader, nullptr);
    vkDestroyShaderModule(m_vulkanEngine.getDevice(), blockFragmentShader, nullptr);
}

void Renderer::cleanupBlockPipeline()
{
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_blockPipelineLayout, nullptr);
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_blockPipeline, nullptr);
}

void Renderer::createPipelines()
{
    createSkyPipeline();
    createBlockPipeline();
}

void Renderer::cleanupPipelines()
{
    cleanupSkyPipeline();
    cleanupBlockPipeline();
}

void Renderer::drawSky()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    transitionImage(command, m_skyImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, m_skyPipeline);
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_COMPUTE, m_skyPipelineLayout,
        0, 1, &m_skyImageDescriptors,
        0, nullptr
    );

    vkCmdPushConstants(
        command, m_skyPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SkyPushConstants),
        &skyRenderInfo
    );

    vkCmdDispatch(
        command, (m_vulkanEngine.getRenderExtent().width + 15) / 16,
        (m_vulkanEngine.getRenderExtent().height + 15) / 16, 1
    );

    transitionImage(
        command, m_skyImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );
    transitionImage(
        command, m_vulkanEngine.getDrawImage().image, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    blitImageToImage(
        command, m_skyImage.image, m_vulkanEngine.getDrawImage().image,
        m_vulkanEngine.getRenderExtent(), m_vulkanEngine.getRenderExtent(), VK_FILTER_NEAREST
    );

    transitionImage(
        command, m_skyImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    transitionImage(
        command, m_vulkanEngine.getDrawImage().image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
}

void Renderer::drawBlocks(const GPUMeshBuffers& mesh)
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    VkRenderingAttachmentInfo colourAttachment{};
    colourAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colourAttachment.imageView = m_vulkanEngine.getDrawImage().imageView;
    colourAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = { { 0, 0 }, m_vulkanEngine.getRenderExtent() };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colourAttachment;
    renderingInfo.pDepthAttachment = nullptr;

    vkCmdBeginRendering(command, &renderingInfo);
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_blockPipeline);

    VkViewport viewport{};
    viewport.width = m_vulkanEngine.getRenderExtent().width;
    viewport.height = m_vulkanEngine.getRenderExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent.width = m_vulkanEngine.getRenderExtent().width;
    scissor.extent.height = m_vulkanEngine.getRenderExtent().height;

    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdSetScissor(command, 0, 1, &scissor);

    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_blockPipelineLayout,
        0, 1, &m_worldTexturesDescriptors,
        0, nullptr
    );

    blockRenderInfo.vertexBuffer = mesh.vertexBufferAddress;

    vkCmdPushConstants(
        command, m_blockPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BlockPushConstants),
        &blockRenderInfo
    );
    vkCmdBindIndexBuffer(command, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command, mesh.indexCount, 1, 0, 0, 0);
    vkCmdEndRendering(command);
}

void Renderer::beginRenderingFrame()
{
    m_vulkanEngine.startRenderingFrame();

    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    VK_CHECK(vkBeginCommandBuffer(command, &beginInfo));
}

void Renderer::submitFrame()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;
    transitionImage(

        command, m_vulkanEngine.getDrawImage().image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );

    transitionImage(
        command, m_vulkanEngine.getCurrentSwapchainImage(), VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    blitImageToImage(
        command, m_vulkanEngine.getDrawImage().image, m_vulkanEngine.getCurrentSwapchainImage(),
        m_vulkanEngine.getRenderExtent(), m_vulkanEngine.getRenderExtent(), VK_FILTER_NEAREST
    );

    transitionImage(
        command, m_vulkanEngine.getCurrentSwapchainImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );

    VK_CHECK(vkEndCommandBuffer(command));

    m_vulkanEngine.submitFrame();
}

}  // namespace lonelycube::client
