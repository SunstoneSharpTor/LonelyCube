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

#include "client/graphics/renderer.h"

#include "client/graphics/vulkan/vulkanEngine.h"

#include "client/graphics/vulkan/images.h"
#include "client/graphics/vulkan/pipelines.h"
#include "client/graphics/vulkan/shaders.h"
#include "client/graphics/vulkan/utils.h"
#include "core/log.h"
#include <volk.h>
#include <vulkan/vulkan_core.h>

namespace lonelycube::client {

Renderer::Renderer(VkSampleCountFlagBits numSamples, float renderScale) :
    m_renderScale(renderScale), m_autoExposure(m_vulkanEngine), m_bloom(m_vulkanEngine)
{
    m_vulkanEngine.init();

    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
    {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    };
    m_globalDescriptorAllocator.init(m_vulkanEngine.getDevice(), 10, sizes);

    m_numSamples = m_vulkanEngine.getMaxSamples() < numSamples ?
        m_vulkanEngine.getMaxSamples() : numSamples;
    if (m_vulkanEngine.getPhysicalDeviceFeatures().features.sampleRateShading == VK_FALSE)
        m_numSamples = VK_SAMPLE_COUNT_1_BIT;

    createRenderImages();
    createSamplers();
    m_autoExposure.init(m_globalDescriptorAllocator, m_drawImage.imageView, m_linearFullscreenSampler);
    m_bloom.init(m_globalDescriptorAllocator, m_drawImage, m_linearFullscreenSampler);
    loadTextures();
    createDescriptors();
    createPipelines();
}

Renderer::~Renderer()
{
    vkDeviceWaitIdle(m_vulkanEngine.getDevice());

    m_autoExposure.cleanup();
    m_bloom.cleanup();

    cleanupPipelines();
    cleanupDescriptors();
    cleanupRenderImages();
    cleanupTextures();
    cleanupSamplers();
    m_globalDescriptorAllocator.destroyPools(m_vulkanEngine.getDevice());

    m_vulkanEngine.cleanup();
}

void Renderer::createRenderImages()
{
    m_drawImageExtent.width = std::ceil(
        m_vulkanEngine.getSwapchainExtent().width * m_renderScale);
    m_drawImageExtent.height = std::ceil(
        m_vulkanEngine.getSwapchainExtent().height * m_renderScale);
    m_drawImageExtent.depth = 1;

    VkImageUsageFlags drawImageUsages = VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    m_drawImage = m_vulkanEngine.createImage(
        m_drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages
    );

    if (m_numSamples > VK_SAMPLE_COUNT_1_BIT)
    {
        m_multisampledDrawImage = m_vulkanEngine.createImage(
            m_drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 1,
            m_numSamples
        );
    }

    VkImageUsageFlags skyImageUsages = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    m_skyImage = m_vulkanEngine.createImage(
        m_drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, skyImageUsages
    );
    m_skyBackgroundImage = m_vulkanEngine.createImage(
        m_drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, skyImageUsages
    );

    m_depthImage = m_vulkanEngine.createImage(
        m_drawImageExtent, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1,
        m_numSamples
    );
}

void Renderer::cleanupRenderImages()
{
    m_vulkanEngine.destroyImage(m_skyImage);
    m_vulkanEngine.destroyImage(m_skyBackgroundImage);
    m_vulkanEngine.destroyImage(m_drawImage);
    if (m_numSamples != VK_SAMPLE_COUNT_1_BIT)
        m_vulkanEngine.destroyImage(m_multisampledDrawImage);
    m_vulkanEngine.destroyImage(m_depthImage);
}

void Renderer::loadTextures()
{
    int size[2];
    int channels;
    uint8_t* buffer = stbi_load("res/resourcePack/textures.png", &size[0], &size[1], &channels, 4);
    VkExtent3D extent { static_cast<uint32_t>(size[0]), static_cast<uint32_t>(size[1]), 1 };
    m_worldTextures = m_vulkanEngine.createImage(
        buffer, extent, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, 5
    );
    stbi_image_free(buffer);

    buffer = stbi_load("res/resourcePack/gui/crosshair.png", &size[0], &size[1], &channels, 1);
    extent = { static_cast<uint32_t>(size[0]), static_cast<uint32_t>(size[1]), 1 };
    m_crosshairTexture = m_vulkanEngine.createImage(
        buffer, extent, VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT
    );
    stbi_image_free(buffer);
}

void Renderer::cleanupTextures()
{
    m_vulkanEngine.destroyImage(m_worldTextures);
    m_vulkanEngine.destroyImage(m_crosshairTexture);
}

void Renderer::createSamplers()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    vkCreateSampler(m_vulkanEngine.getDevice(), &samplerInfo, nullptr, &m_nearestFullscreenSampler);

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    vkCreateSampler(m_vulkanEngine.getDevice(), &samplerInfo, nullptr, &m_linearFullscreenSampler);

    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    // samplerInfo.anisotropyEnable =
    //     m_vulkanEngine.getPhysicalDeviceProperties().properties.limits.maxSamplerAnisotropy != 0.0f;
    // samplerInfo.maxAnisotropy =
    //     m_vulkanEngine.getPhysicalDeviceProperties().properties.limits.maxSamplerAnisotropy;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.mipLodBias = -0.0f;

    vkCreateSampler(m_vulkanEngine.getDevice(), &samplerInfo, nullptr, &m_worldTexturesSampler);

    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.mipLodBias = 0.0f;

    vkCreateSampler(m_vulkanEngine.getDevice(), &samplerInfo, nullptr, &m_uiSampler);
}

void Renderer::cleanupSamplers()
{
    vkDestroySampler(m_vulkanEngine.getDevice(), m_nearestFullscreenSampler, nullptr);
    vkDestroySampler(m_vulkanEngine.getDevice(), m_linearFullscreenSampler, nullptr);

    vkDestroySampler(m_vulkanEngine.getDevice(), m_worldTexturesSampler, nullptr);
    vkDestroySampler(m_vulkanEngine.getDevice(), m_uiSampler, nullptr);
}

void Renderer::createSkyDescriptors()
{
    DescriptorLayoutBuilder builder;

    builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    builder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    m_skyImageDescriptorLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_COMPUTE_BIT
    );

    m_skyImageDescriptors = m_globalDescriptorAllocator.allocate(
        m_vulkanEngine.getDevice(), m_skyImageDescriptorLayout
    );

    builder.clear();
    builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    m_skyBackgroundDescriptorLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT
    );

    m_skyBackgroundDescriptors = m_globalDescriptorAllocator.allocate(
        m_vulkanEngine.getDevice(), m_skyBackgroundDescriptorLayout
    );

    updateSkyDescriptors();
}

void Renderer::updateSkyDescriptors()
{
    DescriptorWriter writer;

    writer.writeImage(
        0, m_skyImage.imageView, VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_IMAGE_LAYOUT_GENERAL
    );
    writer.writeImage(
        1, m_skyBackgroundImage.imageView, VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_IMAGE_LAYOUT_GENERAL
    );
    writer.updateSet(m_vulkanEngine.getDevice(), m_skyImageDescriptors);

    writer.clear();
    writer.writeImage(
        0, m_skyBackgroundImage.imageView, VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_IMAGE_LAYOUT_GENERAL
    );
    writer.updateSet(m_vulkanEngine.getDevice(), m_skyBackgroundDescriptors);
}

void Renderer::createWorldDescriptors()
{
    DescriptorLayoutBuilder builder;

    builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    builder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    m_worldTexturesDescriptorLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT
    );

    m_worldTexturesDescriptors = m_globalDescriptorAllocator.allocate(
        m_vulkanEngine.getDevice(), m_worldTexturesDescriptorLayout
    );

    updateWorldDescriptors();
}

void Renderer::updateWorldDescriptors()
{
    DescriptorWriter writer;

    writer.writeImage(
        0, m_worldTextures.imageView, m_worldTexturesSampler,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    writer.writeImage(
        1, m_skyImage.imageView, m_nearestFullscreenSampler,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    writer.updateSet(m_vulkanEngine.getDevice(), m_worldTexturesDescriptors);
}

void Renderer::createToneMapDescriptors()
{
    DescriptorLayoutBuilder builder;

    builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    m_toneMapDescriptorLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT
    );

    m_toneMapDescriptors = m_globalDescriptorAllocator.allocate(
        m_vulkanEngine.getDevice(), m_toneMapDescriptorLayout
    );

    updateToneMapDescriptors();
}

void Renderer::updateToneMapDescriptors()
{
    DescriptorWriter writer;
    writer.writeImage(
        0, m_drawImage.imageView,
        m_renderScale > 1.0f ? m_linearFullscreenSampler : m_nearestFullscreenSampler,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    writer.updateSet(m_vulkanEngine.getDevice(), m_toneMapDescriptors);
}

void Renderer::createCrosshairDescriptors()
{
    DescriptorLayoutBuilder builder;
    DescriptorWriter writer;

    builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    m_crosshairDescriptorLayout = builder.build(
        m_vulkanEngine.getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT
    );

    m_crosshairDescriptors = m_globalDescriptorAllocator.allocate(
        m_vulkanEngine.getDevice(), m_crosshairDescriptorLayout
    );

    writer.writeImage(
        0, m_crosshairTexture.imageView, m_uiSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    writer.updateSet(m_vulkanEngine.getDevice(), m_crosshairDescriptors);
}

void Renderer::createDescriptors()
{
    createSkyDescriptors();
    createWorldDescriptors();
    createToneMapDescriptors();
    createCrosshairDescriptors();
}

void Renderer::cleanupDescriptors()
{
    vkDestroyDescriptorSetLayout(m_vulkanEngine.getDevice(), m_skyImageDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(
        m_vulkanEngine.getDevice(), m_skyBackgroundDescriptorLayout, nullptr
    );
    vkDestroyDescriptorSetLayout(
        m_vulkanEngine.getDevice(), m_worldTexturesDescriptorLayout, nullptr
    );
    vkDestroyDescriptorSetLayout(m_vulkanEngine.getDevice(), m_toneMapDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_vulkanEngine.getDevice(), m_crosshairDescriptorLayout, nullptr);
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

    VkComputePipelineCreateInfo skyPipelineCreateInfo{};
    skyPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    skyPipelineCreateInfo.layout = m_skyPipelineLayout;
    skyPipelineCreateInfo.stage = stageInfo;

    VK_CHECK(vkCreateComputePipelines(
        m_vulkanEngine.getDevice(), VK_NULL_HANDLE, 1, &skyPipelineCreateInfo, nullptr,
        &m_skyPipeline
    ));

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), computeDrawShader, nullptr);
}

void Renderer::cleanupSkyPipeline()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_skyPipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_skyPipelineLayout, nullptr);
}

void Renderer::createSkyBlitPipeline()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_skyBackgroundDescriptorLayout;

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr, &m_fullscreenBlitPipelineLayout
    ));

    VkShaderModule fullscreenVertexShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/fullscreen.vert.spv", fullscreenVertexShader)
    ) {
        LOG("Failed to find shader \"res/shaders/fullscreen.vert.spv\"");
    }
    VkShaderModule blitFragmentShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/blit.frag.spv", blitFragmentShader)
    ) {
        LOG("Failed to find shader \"res/shaders/blit.frag.spv\"");
    }

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.pipelineLayout = m_fullscreenBlitPipelineLayout;
    pipelineBuilder.setShaders(fullscreenVertexShader, blitFragmentShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.disableBlending();
    pipelineBuilder.disableDepthTest();
    pipelineBuilder.setDepthAttachmentFormat(m_depthImage.imageFormat);
    if (m_numSamples == VK_SAMPLE_COUNT_1_BIT)
    {
        pipelineBuilder.setColourAttachmentFormat(m_drawImage.imageFormat);
        pipelineBuilder.setMultisamplingNone();
    }
    else
    {
        pipelineBuilder.setColourAttachmentFormat(m_multisampledDrawImage.imageFormat);
        pipelineBuilder.setMultisampling(m_numSamples);
    }

    m_skyBlitPipeline = pipelineBuilder.buildPipeline(m_vulkanEngine.getDevice());

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), fullscreenVertexShader, nullptr);
    vkDestroyShaderModule(m_vulkanEngine.getDevice(), blitFragmentShader, nullptr);
}

void Renderer::cleanupSkyBlitPipeline()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_skyBlitPipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_fullscreenBlitPipelineLayout, nullptr);
}

void Renderer::createWorldPipelines()
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

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr, &m_worldPipelineLayout
    ));

    VkShaderModule blockVertexShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/block.vert.spv", blockVertexShader)
    ) {
        LOG("Failed to find shader \"res/shaders/block.vert.spv\"");
    }
    VkShaderModule blockFragmentShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/block.frag.spv", blockFragmentShader)
    ) {
        LOG("Failed to find shader \"res/shaders/block.frag.spv\"");
    }
    VkShaderModule waterVertexShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/water.vert.spv", waterVertexShader)
    ) {
        LOG("Failed to find shader \"res/shaders/water.vert.spv\"");
    }
    VkShaderModule waterFragmentShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/water.frag.spv", waterFragmentShader)
    ) {
        LOG("Failed to find shader \"res/shaders/water.frag.spv\"");
    }

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.pipelineLayout = m_worldPipelineLayout;
    pipelineBuilder.setShaders(blockVertexShader, blockFragmentShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.disableBlending();
    pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineBuilder.setDepthAttachmentFormat(m_depthImage.imageFormat);
    if (m_numSamples == VK_SAMPLE_COUNT_1_BIT)
    {
        pipelineBuilder.setColourAttachmentFormat(m_drawImage.imageFormat);
        pipelineBuilder.setMultisamplingNone();
    }
    else
    {
        pipelineBuilder.setColourAttachmentFormat(m_multisampledDrawImage.imageFormat);
        pipelineBuilder.setMultisampling(m_numSamples);
    }

    m_blockPipeline = pipelineBuilder.buildPipeline(m_vulkanEngine.getDevice());

    pipelineBuilder.setShaders(waterVertexShader, waterFragmentShader);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.enableAlphaBlending();
    pipelineBuilder.enableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

    m_waterPipeline = pipelineBuilder.buildPipeline(m_vulkanEngine.getDevice());

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), blockVertexShader, nullptr);
    vkDestroyShaderModule(m_vulkanEngine.getDevice(), blockFragmentShader, nullptr);
    vkDestroyShaderModule(m_vulkanEngine.getDevice(), waterVertexShader, nullptr);
    vkDestroyShaderModule(m_vulkanEngine.getDevice(), waterFragmentShader, nullptr);
}

void Renderer::cleanupWorldPipelines()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_blockPipeline, nullptr);
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_waterPipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_worldPipelineLayout, nullptr);
}

void Renderer::createBlockOutlinePipeline()
{
    VkPushConstantRange bufferRange{};
    bufferRange.size = 8 * sizeof(glm::vec4);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &bufferRange;

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr, &m_blockOutlinePipelineLayout
    ));

    VkShaderModule vertexShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/blockOutline.vert.spv", vertexShader)
    ) {
        LOG("Failed to find shader \"res/shaders/blockOutline.vert.spv\"");
    }
    VkShaderModule fragmentShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/blockOutline.frag.spv", fragmentShader)
    ) {
        LOG("Failed to find shader \"res/shaders/blockOutline.frag.spv\"");
    }

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.pipelineLayout = m_blockOutlinePipelineLayout;
    pipelineBuilder.setShaders(vertexShader, fragmentShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.disableBlending();
    pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineBuilder.setColourAttachmentFormat(m_drawImage.imageFormat);
    pipelineBuilder.setDepthAttachmentFormat(m_depthImage.imageFormat);
    if (m_numSamples == VK_SAMPLE_COUNT_1_BIT)
    {
        pipelineBuilder.setColourAttachmentFormat(m_drawImage.imageFormat);
        pipelineBuilder.setMultisamplingNone();
    }
    else
    {
        pipelineBuilder.setColourAttachmentFormat(m_multisampledDrawImage.imageFormat);
        pipelineBuilder.setMultisampling(m_numSamples);
    }

    m_blockOutlinePipeline = pipelineBuilder.buildPipeline(m_vulkanEngine.getDevice());

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), vertexShader, nullptr);
    vkDestroyShaderModule(m_vulkanEngine.getDevice(), fragmentShader, nullptr);
}

void Renderer::cleanupBlockOutlinePipeline()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_blockOutlinePipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_blockOutlinePipelineLayout, nullptr);
}

void Renderer::createToneMapPipeline()
{
    VkPushConstantRange bufferRange{};
    bufferRange.size = sizeof(ToneMapPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &bufferRange;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_toneMapDescriptorLayout;

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr, &m_toneMapPipelineLayout
    ));

    VkShaderModule fullscreenVertexShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/fullscreen.vert.spv", fullscreenVertexShader)
    ) {
        LOG("Failed to find shader \"res/shaders/fullscreen.vert.spv\"");
    }
    VkShaderModule toneMapFragmentShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/toneMap.frag.spv", toneMapFragmentShader)
    ) {
        LOG("Failed to find shader \"res/shaders/toneMap.frag.spv\"");
    }

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.pipelineLayout = m_toneMapPipelineLayout;
    pipelineBuilder.setShaders(fullscreenVertexShader, toneMapFragmentShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.disableBlending();
    pipelineBuilder.disableDepthTest();
    pipelineBuilder.setColourAttachmentFormat(m_vulkanEngine.getSwapchainImageFormat());
    pipelineBuilder.setDepthAttachmentFormat(VK_FORMAT_UNDEFINED);

    m_toneMapPipeline = pipelineBuilder.buildPipeline(m_vulkanEngine.getDevice());

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), fullscreenVertexShader, nullptr);
    vkDestroyShaderModule(m_vulkanEngine.getDevice(), toneMapFragmentShader, nullptr);

    m_toneMapPushConstants.drawImageTexelSize.x = 1.0f / m_drawImageExtent.width;
    m_toneMapPushConstants.drawImageTexelSize.y = 1.0f / m_drawImageExtent.height;
    m_toneMapPushConstants.luminanceBuffer = m_autoExposure.getExposureBuffer();
}

void Renderer::cleanupToneMapPipeline()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_toneMapPipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_toneMapPipelineLayout, nullptr);
}

void Renderer::createCrosshairPipeline()
{
    VkPushConstantRange bufferRange{};
    bufferRange.size = sizeof(glm::vec2);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &bufferRange;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_crosshairDescriptorLayout;

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &pipelineLayoutInfo, nullptr, &m_crosshairPipelineLayout
    ));

    VkShaderModule crosshairVertexShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/crosshair.vert.spv", crosshairVertexShader)
    ) {
        LOG("Failed to find shader \"res/shaders/crosshair.vert.spv\"");
    }
    VkShaderModule crosshairFragmentShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/crosshair.frag.spv", crosshairFragmentShader)
    ) {
        LOG("Failed to find shader \"res/shaders/crosshair.frag.spv\"");
    }

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.pipelineLayout = m_crosshairPipelineLayout;
    pipelineBuilder.setShaders(crosshairVertexShader, crosshairFragmentShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.enableNegativeBlending();
    pipelineBuilder.disableDepthTest();
    pipelineBuilder.setColourAttachmentFormat(m_vulkanEngine.getSwapchainImageFormat());
    pipelineBuilder.setDepthAttachmentFormat(VK_FORMAT_UNDEFINED);

    m_crosshairPipeline = pipelineBuilder.buildPipeline(m_vulkanEngine.getDevice());

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), crosshairVertexShader, nullptr);
    vkDestroyShaderModule(m_vulkanEngine.getDevice(), crosshairFragmentShader, nullptr);
}

void Renderer::cleanupCrosshairPipeline()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_crosshairPipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_crosshairPipelineLayout, nullptr);
}

void Renderer::createPipelines()
{
    createSkyPipeline();
    createSkyBlitPipeline();
    createWorldPipelines();
    createBlockOutlinePipeline();
    createToneMapPipeline();
    createCrosshairPipeline();
}

void Renderer::cleanupPipelines()
{
    cleanupSkyPipeline();
    cleanupSkyBlitPipeline();
    cleanupWorldPipelines();
    cleanupBlockOutlinePipeline();
    cleanupToneMapPipeline();
    cleanupCrosshairPipeline();
}

void Renderer::beginRenderingFrame()
{
    VkExtent2D swapchainExtent;
    m_vulkanEngine.startRenderingFrame(swapchainExtent);
    VkExtent2D newRenderExtent;
    newRenderExtent.width = std::ceil(swapchainExtent.width * m_renderScale);
    newRenderExtent.height = std::ceil(swapchainExtent.height * m_renderScale);
    if (newRenderExtent.width != m_drawImageExtent.width ||
        newRenderExtent.height != m_drawImageExtent.height)
    {
        resize();
    }

    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    VK_CHECK(vkBeginCommandBuffer(command, &beginInfo));
    #ifdef TIMESTAMPS
    vkCmdResetQueryPool(
        command, m_vulkanEngine.getCurrentTimestampQueryPool(), 0,
        static_cast<uint32_t>(m_vulkanEngine.getTimestamps().size())
    );
    LOG(std::to_string(m_vulkanEngine.getDeltaTimestamp(0, 1)));
    #endif
}

void Renderer::drawSky()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    transitionImage(
        command, m_skyImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_MEMORY_WRITE_BIT
    );
    transitionImage(
        command, m_skyBackgroundImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_MEMORY_WRITE_BIT
    );

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, m_skyPipeline);
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_COMPUTE, m_skyPipelineLayout,
        0, 1, &m_skyImageDescriptors,
        0, nullptr
    );

    skyRenderInfo.renderSize = glm::vec2(m_drawImageExtent.width, m_drawImageExtent.height);
    vkCmdPushConstants(
        command, m_skyPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SkyPushConstants),
        &skyRenderInfo
    );

    vkCmdDispatch(
        command, (m_drawImageExtent.width + 15) / 16, (m_drawImageExtent.height + 15) / 16, 1
    );

    transitionImage(
        command, m_skyImage.image, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_MEMORY_READ_BIT
    );
    transitionImage(
        command, m_skyBackgroundImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_MEMORY_READ_BIT
    );
}

void Renderer::updateEntityMesh(
    const EntityMeshManager& entityMeshManager, GPUDynamicMeshBuffers& mesh
) {
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    m_vulkanEngine.updateDynamicMesh(
        command, mesh, entityMeshManager.sizeOfVertices, entityMeshManager.numIndices
    );
}

void Renderer::beginDrawingGeometry()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    transitionImage(
        command, m_drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_MEMORY_READ_BIT
    );
    transitionImage(
        command, m_depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT
    );

    VkRenderingAttachmentInfo colourAttachment{};
    colourAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colourAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    if (m_numSamples == VK_SAMPLE_COUNT_1_BIT)
    {
        colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colourAttachment.imageView = m_drawImage.imageView;
    }
    else
    {
        colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colourAttachment.imageView = m_multisampledDrawImage.imageView;
        colourAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        colourAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colourAttachment.resolveImageView = m_drawImage.imageView;
    }

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = m_depthImage.imageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil.depth = 0.0f;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {
        { 0, 0 }, VkExtent2D(m_drawImageExtent.width, m_drawImageExtent.height) 
    };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colourAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(command, &renderingInfo);
}

void Renderer::blitSky()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyBlitPipeline);

    VkViewport viewport{};
    viewport.width = m_drawImageExtent.width;
    viewport.height = m_drawImageExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent.width = m_drawImageExtent.width;
    scissor.extent.height = m_drawImageExtent.height;

    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdSetScissor(command, 0, 1, &scissor);

    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_fullscreenBlitPipelineLayout,
        0, 1, &m_skyBackgroundDescriptors,
        0, nullptr
    );

    vkCmdDraw(command, 3, 1, 0, 0);
}

void Renderer::beginDrawingBlocks()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_blockPipeline);

    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_worldPipelineLayout,
        0, 1, &m_worldTexturesDescriptors,
        0, nullptr
    );
}

void Renderer::beginDrawingWater()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_waterPipeline);
}

void Renderer::drawBlocks(const GPUMeshBuffers& mesh)
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    blockRenderInfo.vertexBuffer = mesh.vertexBufferAddress;

    vkCmdPushConstants(
        command, m_worldPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BlockPushConstants),
        &blockRenderInfo
    );
    vkCmdBindIndexBuffer(command, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command, mesh.indexCount, 1, 0, 0, 0);
}

void Renderer::drawEntities(GPUDynamicMeshBuffers& mesh)
{
    FrameData& currentFrameData = getVulkanEngine().getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    blockRenderInfo.vertexBuffer = mesh.vertexBufferAddress;
    vkCmdPushConstants(
        command, m_worldPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BlockPushConstants),
        &blockRenderInfo
    );
    vkCmdBindIndexBuffer(
        command, mesh.indexBuffer.deviceLocalBuffer.buffer, 0, VK_INDEX_TYPE_UINT32
    );
    vkCmdDrawIndexed(command, mesh.indexCount, 1, 0, 0, 0);
}

void Renderer::drawBlockOutline(glm::mat4& viewProjection, glm::vec3& offset, float* outlineModel)
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_blockOutlinePipeline);

    std::array<glm::vec4, 8> outlineVertices;
    for (int i = 0; i < 8; i++)
    {
        glm::vec4 vertexCoords(
            outlineModel[i * 3], outlineModel[i * 3 + 1], outlineModel[i * 3 + 2], 1.0f
        );
        vertexCoords += glm::vec4(offset, 0.0f);
        outlineVertices[i] = viewProjection * glm::vec4(vertexCoords);
        outlineVertices[i].z *= 1.004f;  // Increase the depth slightly to prevent z-fighting
    }

    vkCmdPushConstants(
        command, m_blockOutlinePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 8 * sizeof(glm::vec4),
        outlineVertices.data()
    );
    vkCmdDraw(command, 16, 1, 0, 0);
}

void Renderer::finishDrawingGeometry()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    vkCmdEndRendering(command);
}

void Renderer::renderBloom()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    transitionImage(
        command, m_drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_MEMORY_READ_BIT
    );

    m_bloom.render(5.0f, 0.006f);
}

void Renderer::calculateAutoExposure(double DT)
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    transitionImage(
        command, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_MEMORY_READ_BIT
    );

    glm::vec2 renderAreaFraction(
        static_cast<float>(m_drawImageExtent.width) / m_drawImageExtent.width,
         static_cast<float>(m_drawImageExtent.height) / m_drawImageExtent.height
    );
    m_autoExposure.calculate(renderAreaFraction, DT);
}

void Renderer::applyToneMap()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    transitionImage(
        command, m_vulkanEngine.getCurrentSwapchainImage(), VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT
    );

    VkRenderingAttachmentInfo colourAttachment{};
    colourAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colourAttachment.imageView = m_vulkanEngine.getCurrentSwapchainImageView();
    colourAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = { { 0, 0 }, m_vulkanEngine.getSwapchainExtent() };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colourAttachment;

    vkCmdBeginRendering(command, &renderingInfo);
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_toneMapPipeline);

    VkViewport viewport{};
    viewport.width = m_vulkanEngine.getSwapchainExtent().width;
    viewport.height = m_vulkanEngine.getSwapchainExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent.width = m_vulkanEngine.getSwapchainExtent().width;
    scissor.extent.height = m_vulkanEngine.getSwapchainExtent().height;

    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdSetScissor(command, 0, 1, &scissor);

    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_toneMapPipelineLayout,
        0, 1, &m_toneMapDescriptors,
        0, nullptr
    );

    vkCmdPushConstants(
        command, m_toneMapPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        sizeof(ToneMapPushConstants), &m_toneMapPushConstants
    );
    vkCmdDraw(command, 3, 1, 0, 0);
}

void Renderer::drawCrosshair()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    VkRenderingAttachmentInfo colourAttachment{};
    colourAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colourAttachment.imageView = m_vulkanEngine.getCurrentSwapchainImageView();
    colourAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = { { 0, 0 }, m_vulkanEngine.getSwapchainExtent() };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colourAttachment;

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_crosshairPipeline);

    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_crosshairPipelineLayout,
        0, 1, &m_crosshairDescriptors,
        0, nullptr
    );

    glm::vec2 size{ 36.0f / m_vulkanEngine.getSwapchainExtent().width,
                    36.0f / m_vulkanEngine.getSwapchainExtent().height };
    vkCmdPushConstants(
        command, m_crosshairPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec2),
        &size
    );
    vkCmdDraw(command, 6, 1, 0, 0);
    vkCmdEndRendering(command);

    transitionImage(
        command, m_vulkanEngine.getCurrentSwapchainImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VK_ACCESS_2_MEMORY_READ_BIT
    );
}

void Renderer::submitFrame()
{
    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    VK_CHECK(vkEndCommandBuffer(command));
    m_vulkanEngine.submitFrame();
}

void Renderer::resize()
{
    LOG("RESIZING");
    cleanupRenderImages();
    createRenderImages();

    updateSkyDescriptors();
    updateWorldDescriptors();
    m_autoExposure.updateImageView(m_drawImage.imageView, m_linearFullscreenSampler);
    updateToneMapDescriptors();
    m_toneMapPushConstants.drawImageTexelSize.x = 1.0f / m_drawImageExtent.width;
    m_toneMapPushConstants.drawImageTexelSize.y = 1.0f / m_drawImageExtent.height;
    m_bloom.updateSrcImage(m_globalDescriptorAllocator, m_drawImage);
}

}  // namespace lonelycube::client
