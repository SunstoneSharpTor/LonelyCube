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

#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vk_mem_alloc.h"

#include "core/pch.h"

namespace lonelycube::client {

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
};

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct FrameData
{
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore, renderFinishedSemaphore;
    VkFence inFlightFence;
};

struct AllocatedImage
{
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

class VulkanEngine
{
public:
    VulkanEngine();
    bool init();
    void cleanup();
    bool drawFrame();

private:
    const uint32_t m_WIDTH = 800;
    const uint32_t m_HEIGHT = 600;
    const int m_MAX_FRAMES_IN_FLIGHT = 2;
    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    const bool m_enableValidationLayers;

    uint32_t m_currentFrame = 0;
    GLFWwindow* m_window;
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent;
    std::vector<VkImageView> m_swapchainImageViews;
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    std::vector<FrameData> m_frameData;
    bool m_framebufferResized = false;
    VmaAllocator m_allocator;
    AllocatedImage m_drawImage;
    VkExtent2D m_drawExtent;

    void initWindow();
    bool initVulkan();

    void cleanupSwapchain();
    bool createInstance();
    bool checkValidationLayerSupport();
    bool pickPhysicalDevice();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool checkDeviceFeaturesSupport(VkPhysicalDevice device);
    int ratePhysicalDeviceSuitability(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool createLogicalDevice();
    bool createSurface();
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats
    );
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes
    );
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    bool createSwapchain();
    bool createDrawImage();
    bool recreateSwapchain();
    bool createSwapchainImageViews();
    bool createGraphicsPipeline();
    bool createRenderPass();
    bool createFramebuffers();
    bool createFrameData();
    void cleanupFrameData();
    bool createCommandPool(VkCommandPool& commandPool);
    bool createCommandBuffer(VkCommandPool commandPool, VkCommandBuffer& commandBuffer);
    bool recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    bool createSyncObjects(int frameNum);
    bool createAllocator();

public:
    inline VkDevice getDevice()
    {
        return m_device;
    }

    inline GLFWwindow* getWindow()
    {
        return m_window;
    }

    inline void singalFramebufferResize()
    {
        m_framebufferResized = true;
    }
};

}  // namespace lonelycube::client
