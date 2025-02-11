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

#include "client/graphics/vulkan/descriptors.h"

namespace lonelycube::client {

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsAndComputeFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;
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

struct AllocatedBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct GPUMeshBuffers
{
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

class VulkanEngine
{
public:
    DescriptorAllocator globalDescriptorAllocator;

    VulkanEngine();
    void init();
    void cleanup();
    void drawFrame();

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
    VkQueue m_graphicsAndComputeQueue;
    VkQueue m_presentQueue;
    VkQueue m_transferQueue;
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
    VkExtent2D m_drawImageExtent;
    VkExtent2D m_renderExtent;

    // Immediate Submit
    VkFence m_immediateSubmitFence;
    VkCommandBuffer m_immediateSubmitCommandBuffer;
    VkCommandPool m_immediateSubmitCommandPool;

    // Temporary
    VkPipelineLayout m_gradientPipelineLayout;
    VkPipeline m_gradientPipeline;
    VkPipelineLayout m_meshPipelineLayout;
    VkPipeline m_meshPipeline;
    GPUMeshBuffers m_rectangleMesh;

    // Initialisation
    void initWindow();
    void initVulkan();
    bool createInstance();
    bool checkValidationLayerSupport();
    bool pickPhysicalDevice();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool checkDeviceFeaturesSupport(VkPhysicalDevice device);
    int ratePhysicalDeviceSuitability(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void createLogicalDevice();
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats
    );
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes
    );
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapchain();
    void createDrawImage();
    void createSwapchainImageViews();
    void createFrameData();
    void createCommandBuffer(VkCommandPool commandPool, VkCommandBuffer& commandBuffer);
    void createSyncObjects(int frameNum);
    void createAllocator();
    void initDescriptors();
    void initImmediateSubmit();

    // Cleanup
    void cleanupSwapchain();
    void cleanupFrameData();
    void cleanupImmediateSubmit();

    // Resizing
    void recreateSwapchain();

    // Buffers
    AllocatedBuffer createBuffer(
        size_t allocationSize, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags
    );
    inline void destroyBuffer(const AllocatedBuffer& buffer)
    {
        vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
    }
    void immediateSubmit(std::function<void(VkCommandBuffer command)>&& function);

    // Temporary
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void drawBackgroud(VkCommandBuffer command);
    void drawGeometry(VkCommandBuffer command);
    void initPipelines();
    void cleanupPipelines();
    void initBackgroundPipelines();
    void cleanupBackgroundPipelines();
    void initMeshPipeline();
    void cleanupMeshPipeline();
    GPUMeshBuffers uploadMesh(std::span<float> vertices, std::span<uint32_t> indices);
    void uploadTestMesh();

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
