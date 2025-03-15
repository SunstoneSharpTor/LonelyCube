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

#include "client/graphics/entityMeshManager.h"
#include "glm/glm.hpp"
#include "stb_image.h"

#include "client/graphics/vulkan/vulkanEngine.h"
#include "client/graphics/vulkan/descriptors.h"

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
    glm::vec3 cameraOffset;
    float renderDistance;
    VkDeviceAddress vertexBuffer;
    float skyLightIntensity;
};

struct ExposurePushConstants
{
    glm::vec2 inverseDrawImageSize;
    float exposure;
};

class Renderer
{
public:
    SkyPushConstants skyRenderInfo;
    BlockPushConstants blockRenderInfo;
    float exposure;

    Renderer(float renderScale);
    ~Renderer();
    void beginRenderingFrame();
    void drawSky();
    void drawSun();
    void beginDrawingGeometry();
    void beginDrawingBlocks();
    void drawBlocks(const GPUMeshBuffers& mesh);
    void drawEntities(
        const EntityMeshManager& entityMeshManager, GPUDynamicMeshBuffers& mesh
    );
    void beginDrawingWater();
    void drawBlockOutline(glm::mat4& viewProjection, glm::vec3& offset, float* outlineModel);
    void finishDrawingGeometry();
    void applyExposure();
    void drawCrosshair();
    void submitFrame();

    inline VulkanEngine& getVulkanEngine()
    {
        return m_vulkanEngine;
    }

private:
    VulkanEngine m_vulkanEngine;
    DescriptorAllocatorGrowable m_globalDescriptorAllocator;

    AllocatedImage m_drawImage;
    AllocatedImage m_depthImage;
    float m_renderScale;
    VkExtent2D m_maxWindowExtent;
    VkExtent3D m_drawImageExtent;
    VkExtent2D m_renderExtent;

    AllocatedImage m_skyImage;
    VkDescriptorSetLayout m_skyImageDescriptorLayout;
    VkDescriptorSet m_skyImageDescriptors;
    VkPipelineLayout m_skyPipelineLayout;
    VkPipeline m_skyPipeline;

    VkDescriptorSetLayout m_drawImageDescriptorLayout;
    VkDescriptorSet m_drawImageDescriptors;
    VkPipelineLayout m_sunPipelineLayout;
    VkPipeline m_sunPipeline;

    VkDescriptorSetLayout m_worldTexturesDescriptorLayout;
    VkDescriptorSet m_worldTexturesDescriptors;
    VkPipelineLayout m_worldPipelineLayout;
    VkPipeline m_blockPipeline;
    VkPipeline m_waterPipeline;

    VkDescriptorSetLayout m_exposureDescriptorLayout;
    VkDescriptorSet m_exposureDescriptors;
    VkPipelineLayout m_exposurePipelineLayout;
    VkPipeline m_exposurePipeline;

    VkPipelineLayout m_blockOutlinePipelineLayout;
    VkPipeline m_blockOutlinePipeline;

    VkDescriptorSetLayout m_crosshairDescriptorLayout;
    VkDescriptorSet m_crosshairDescriptors;
    VkPipelineLayout m_crosshairPipelineLayout;
    VkPipeline m_crosshairPipeline;

    AllocatedImage m_worldTextures;
    AllocatedImage m_crosshairTexture;

    VkSampler m_worldTexturesSampler;
    VkSampler m_linearFullscreenSampler;
    VkSampler m_nearestFullscreenSampler;
    VkSampler m_uiSampler;

    void createSkyPipeline();
    void cleanupSkyPipeline();
    void createSunPipeline();
    void cleanupSunPipeline();
    void createWorldPipelines();
    void cleanupWorldPipelines();
    void createBlockOutlinePipeline();
    void cleanupBlockOutlinePipeline();
    void createExposurePipeline();
    void cleanupExposurePipeline();
    void createCrosshairPipeline();
    void cleanupCrosshairPipeline();
    void createPipelines();
    void cleanupPipelines();

    void createDrawImageDescriptors();
    void createSkyDescriptors();
    void createWorldDescriptors();
    void createExposureDescriptors();
    void createCrosshairDescriptors();
    void createDescriptors();
    void cleanupDescriptors();

    void loadTextures();
    void cleanupTextures();

    void createDrawImage();
    void createDepthImage();
    void cleanupDepthImage();
    void createSkyImage();
    void cleanupSkyImage();
    void createFullscreenSamplers();
    void cleanupFullscreenSamplers();
};

}  // namespace lonelycube::client
