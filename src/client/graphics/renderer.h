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
#include "stb_image.h"

#include "client/graphics/entityMeshManager.h"
#include "client/graphics/autoExposure.h"
#include "client/graphics/bloom.h"
#include "client/graphics/vulkan/vulkanEngine.h"
#include "client/graphics/vulkan/descriptors.h"
#include "client/gui/font.h"
#include "client/gui/menuRenderer.h"

namespace lonelycube::client {

struct SkyPushConstants
{
    glm::vec3 sunDir;
    float brightness;
    glm::mat4 inverseViewProjection;
    glm::vec3 sunGlowColour;
    float sunGlowAmount;
    glm::vec2 renderSize;
};

struct BlockPushConstants
{
    glm::mat4 mvp;
    glm::vec3 playerSubBlockPos;
    float renderDistance;
    glm::vec3 chunkCoordinates;
    float skyLightIntensity;
    VkDeviceAddress vertexBuffer;
};

struct ToneMapPushConstants
{
    glm::vec2 drawImageTexelSize;
    VkDeviceAddress luminanceBuffer;
    float brightness;
};

struct FullscreenBlitPushConstants
{
    glm::vec2 scale;
    glm::vec2 offset;
};

class Renderer
{
public:
    SkyPushConstants skyRenderInfo;
    BlockPushConstants blockRenderInfo;
    Font font;
    MenuRenderer menuRenderer;

    Renderer(VkSampleCountFlagBits numSamples, float renderScale);
    ~Renderer();
    void beginRenderingFrame();
    void drawSky();
    void updateEntityMesh(const EntityMeshManager& entityMeshManager, GPUDynamicMeshBuffers& mesh);
    void beginDrawingGeometry();
    void blitSky();
    void beginDrawingBlocks();
    void drawBlocks(const GPUMeshBuffers& mesh);
    void drawEntities(GPUDynamicMeshBuffers& mesh);
    void beginDrawingWater();
    void drawBlockOutline(glm::mat4& viewProjection, glm::vec3& offset, float* outlineModel);
    void finishDrawingGeometry();
    void renderBloom();
    void calculateAutoExposure(double DT);
    void beginRenderingToSwapchainImage();
    void applyToneMap();
    void drawCrosshair();
    void drawBackgroundImage();
    void beginDrawingUi();
    void submitFrame();

    inline VulkanEngine& getVulkanEngine()
    {
        return m_vulkanEngine;
    }
    inline bool isMinimised()
    {
        return m_minimised;
    }
    inline void setGameBrightness(float brightness)
    {
        m_toneMapPushConstants.brightness = brightness;
    }

private:
    VulkanEngine m_vulkanEngine;
    DescriptorAllocatorGrowable m_globalDescriptorAllocator;

    float m_renderScale;
    bool m_minimised;

    AllocatedImage m_drawImage;
    AllocatedImage m_multisampledDrawImage;
    AllocatedImage m_depthImage;
    VkSampleCountFlagBits m_numSamples;
    VkExtent3D m_drawImageExtent;

    AllocatedImage m_skyImage;
    AllocatedImage m_skyBackgroundImage;
    VkDescriptorSetLayout m_skyImageDescriptorLayout;
    VkDescriptorSet m_skyImageDescriptors;
    VkPipelineLayout m_skyPipelineLayout;
    VkPipeline m_skyPipeline;

    VkDescriptorSetLayout m_skyBackgroundDescriptorLayout;
    VkDescriptorSet m_skyBackgroundDescriptors;
    VkPipelineLayout m_fullscreenCopyPipelineLayout;
    VkPipeline m_skyCopyPipeline;

    VkDescriptorSetLayout m_drawImageDescriptorLayout;
    VkDescriptorSet m_drawImageDescriptors;
    VkPipelineLayout m_sunPipelineLayout;
    VkPipeline m_sunPipeline;

    VkDescriptorSetLayout m_worldTexturesDescriptorLayout;
    VkDescriptorSet m_worldTexturesDescriptors;
    VkPipelineLayout m_worldPipelineLayout;
    VkPipeline m_blockPipeline;
    VkPipeline m_waterPipeline;

    VkPipelineLayout m_blockOutlinePipelineLayout;
    VkPipeline m_blockOutlinePipeline;

    AutoExposure m_autoExposure;
    Bloom m_bloom;

    VkDescriptorSetLayout m_toneMapDescriptorLayout;
    VkDescriptorSet m_toneMapDescriptors;
    VkPipelineLayout m_toneMapPipelineLayout;
    VkPipeline m_toneMapPipeline;
    ToneMapPushConstants m_toneMapPushConstants;

    VkPipelineLayout m_uiPipelineLayout;

    VkDescriptorSetLayout m_crosshairDescriptorLayout;
    VkDescriptorSet m_crosshairDescriptors;
    VkPipelineLayout m_crosshairPipelineLayout;
    VkPipeline m_crosshairPipeline;

    VkDescriptorSet m_repeatingLinearSamplerDescriptors;
    VkDescriptorSet m_startMenuBackgroundImageDescriptors;
    VkPipelineLayout m_fullscreenBlitPipelineLayout;
    VkPipeline m_fullscreenBlitPipeline;

    VkDescriptorSetLayout m_uiImageDescriptorLayout;
    VkDescriptorSetLayout m_uiSamplerDescriptorLayout;
    VkDescriptorSet m_uiSamplerDescriptors;

    AllocatedImage m_worldTextures;
    AllocatedImage m_crosshairTexture;
    AllocatedImage m_startMenuBackgroundTexture;

    VkSampler m_worldTexturesSampler;
    VkSampler m_linearFullscreenSampler;
    VkSampler m_nearestFullscreenSampler;
    VkSampler m_uiSampler;
    VkSampler m_repeatingLinearSampler;

    void createSkyPipeline();
    void cleanupSkyPipeline();
    void createSkyBlitPipeline();
    void cleanupSkyBlitPipeline();
    void createWorldPipelines();
    void cleanupWorldPipelines();
    void createBlockOutlinePipeline();
    void cleanupBlockOutlinePipeline();
    void createToneMapPipeline();
    void cleanupToneMapPipeline();
    void createUiPipelines();
    void cleanupUiPipelines();
    void createCrosshairPipeline();
    void cleanupCrosshairPipeline();
    void createFullscreenBlitPipeline();
    void cleanupFullscreenBlitPipeline();
    void createPipelines();
    void cleanupPipelines();

    void createSkyDescriptors();
    void updateSkyDescriptors();
    void createWorldDescriptors();
    void updateWorldDescriptors();
    void createToneMapDescriptors();
    void updateToneMapDescriptors();
    void createCrosshairDescriptors();
    void createUiDescriptors();
    void createDescriptors();
    void cleanupDescriptors();

    void createRenderImages();
    void cleanupRenderImages();
    void createSamplers();
    void cleanupSamplers();
    void loadTextures();
    void cleanupTextures();

    void resize();
};

}  // namespace lonelycube::client
