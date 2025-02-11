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

#include "glm/glm.hpp"

#include "client/graphics/vulkan/shaders.h"
#include "client/graphics/vulkan/utils.h"
#include "core/log.h"

namespace lonelycube::client {

Renderer::Renderer()
{
    m_vulkanEngine.init();
}

Renderer::~Renderer()
{
    m_vulkanEngine.cleanup();
}

void Renderer::drawFrame()
{
    m_vulkanEngine.drawFrame();
}

void Renderer::initSkyPipelines()
{
    VkPipelineLayoutCreateInfo computePipelineLayout{};
    computePipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computePipelineLayout.setLayoutCount = 1;
    computePipelineLayout.pSetLayouts = &m_vulkanEngine.getDrawImageDescriptorSetLayout();

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = 2 * sizeof(glm::vec4);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    computePipelineLayout.pushConstantRangeCount = 1;
    computePipelineLayout.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(
        m_vulkanEngine.getDevice(), &computePipelineLayout, nullptr, &m_skyPipelineLayout
    ));

    VkShaderModule computeDrawShader;
    if (!createShaderModule(
        m_vulkanEngine.getDevice(), "res/shaders/gradient.comp.spv", computeDrawShader))
    {
        LOG("Failed to find shader \"res/shaders/gradient.comp.spv\"");
    }

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = computeDrawShader;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.layout = m_gradientPipelineLayout;
    computePipelineCreateInfo.stage = stageInfo;

    VK_CHECK(vkCreateComputePipelines(
        m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_gradientPipeline
    ));

    vkDestroyShaderModule(m_device, computeDrawShader, nullptr);
}

void Renderer::cleanupSkyPipelines()
{
    vkDestroyPipeline(m_device, m_gradientPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_gradientPipelineLayout, nullptr);
}

void Renderer::initPipelines()
{
    initSkyPipelines();
}

void Renderer::cleanupPipelines()
{
    cleanupSkyPipelines();
}

}  // namespace lonelycube::client
