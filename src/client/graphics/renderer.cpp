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

#include "client/graphics/vulkan/images.h"
#include "client/graphics/vulkan/shaders.h"
#include "client/graphics/vulkan/utils.h"
#include "core/log.h"
#include <vulkan/vulkan_core.h>

namespace lonelycube::client {

Renderer::Renderer()
{
    m_vulkanEngine.init();

    initPipelines();
}

Renderer::~Renderer()
{
    vkDeviceWaitIdle(m_vulkanEngine.getDevice());

    cleanupPipelines();

    m_vulkanEngine.cleanup();
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
    computePipelineCreateInfo.layout = m_skyPipelineLayout;
    computePipelineCreateInfo.stage = stageInfo;

    VK_CHECK(vkCreateComputePipelines(
        m_vulkanEngine.getDevice(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr,
        &m_skyPipeline
    ));

    vkDestroyShaderModule(m_vulkanEngine.getDevice(), computeDrawShader, nullptr);
}

void Renderer::cleanupSkyPipelines()
{
    vkDestroyPipeline(m_vulkanEngine.getDevice(), m_skyPipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanEngine.getDevice(), m_skyPipelineLayout, nullptr);
}

void Renderer::initPipelines()
{
    initSkyPipelines();
}

void Renderer::cleanupPipelines()
{
    cleanupSkyPipelines();
}

void Renderer::drawSky(VkCommandBuffer command)
{
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, m_skyPipeline);
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_COMPUTE, m_skyPipelineLayout,
        0, 1, &m_vulkanEngine.getDrawImageDescriptors(),
        0, nullptr
    );
    double x, y;
    glfwGetCursorPos(m_vulkanEngine.getWindow(), &x, &y);
    x /= m_vulkanEngine.getRenderExtent().width;
    y /= m_vulkanEngine.getRenderExtent().height;
    glm::vec4 colours[2] = { { x, 1 - x, y, 1 }, { 1 - y, y, 1 - x, 1 } };
    vkCmdPushConstants(
        command, m_skyPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 2 * sizeof(glm::vec4), colours
    );
    vkCmdDispatch(
        command, (m_vulkanEngine.getRenderExtent().width + 15) / 16,
        (m_vulkanEngine.getRenderExtent().height + 15) / 16, 1
    );
}

void Renderer::drawFrame()
{
    m_vulkanEngine.startRenderingFrame();

    FrameData& currentFrameData = m_vulkanEngine.getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    VK_CHECK(vkBeginCommandBuffer(command, &beginInfo));

    transitionImage(command, m_vulkanEngine.getDrawImage().image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    drawSky(command);

    transitionImage(
        command, m_vulkanEngine.getDrawImage().image, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    // drawGeometry(command);

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
