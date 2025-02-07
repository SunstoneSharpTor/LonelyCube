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

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vk_mem_alloc.h"

#include "core/pch.h"

#include "client/graphics/vulkan/descriptors.h"

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
    DescriptorAllocator globalDescriptorAllocator;

    VulkanEngine();
    bool init();
    void cleanup();
    bool drawFrame();

private:
    // Engine constants
    const int m_MAX_FRAMES_IN_FLIGHT = 2;
    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    const bool m_enableValidationLayers;

    // Vulkan core objects
    GLFWwindow* m_window;
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkSurfaceKHR m_surface;
    VmaAllocator m_allocator;

    // Swapchain
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent;
    std::vector<VkImageView> m_swapchainImageViews;

    // Descriptors
    VkDescriptorSet m_drawImageDescriptors;
    VkDescriptorSetLayout m_drawImageDescriptorLayout;

    // Rendering
    uint32_t m_currentFrame = 0;
    std::vector<FrameData> m_frameData;
    bool m_windowResized = false;
    AllocatedImage m_drawImage;

    // Temporary
    VkPipelineLayout m_gradientPipelinelayout;
    VkPipeline m_gradientPipeline;

    // Initialisation
    void initWindow();
    bool initVulkan();
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
    bool createSwapchainImageViews();
    bool createFrameData();
    bool createCommandPool(VkCommandPool& commandPool);
    bool createCommandBuffer(VkCommandPool commandPool, VkCommandBuffer& commandBuffer);
    bool createSyncObjects(int frameNum);
    bool createAllocator();
    bool initDescriptors();

    // Cleanup
    void cleanupSwapchain();
    void cleanupFrameData();
    void cleanupBackgroundPipelines();
    void cleanupPipelines();

    bool recreateSwapchain();
    void updateDrawImageDescriptor();

    // Temporary
    bool recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void drawBackgroud(VkCommandBuffer command);
    bool initPipelines();
    bool initBackgroundPipelines();

public:
    inline VkDevice getDevice()
    {
        return m_device;
    }

    inline GLFWwindow* getWindow()
    {
        return m_window;
    }

    inline void singalWindowResize()
    {
        m_windowResized = true;
    }
};

}  // namespace lonelycube::client
