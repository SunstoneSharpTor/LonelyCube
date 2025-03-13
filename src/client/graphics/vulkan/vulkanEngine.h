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

#include <volk.h>
#include "GLFW/glfw3.h"
#include "vk_mem_alloc.h"

#include "core/pch.h"

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

struct AllocatedHostVisibleAndDeviceLocalBuffer
{
    AllocatedBuffer deviceLocalBuffer;
    AllocatedBuffer stagingBuffer;
    void* mappedData;
    bool hostVisibleAndDeviceLocal;
};

struct GPUMeshBuffers
{
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    VkDeviceAddress vertexBufferAddress;
    uint32_t indexCount;
};

struct GPUDynamicMeshBuffers
{
    AllocatedHostVisibleAndDeviceLocalBuffer vertexBuffer;
    AllocatedHostVisibleAndDeviceLocalBuffer indexBuffer;
    VkDeviceAddress vertexBufferAddress;
    uint32_t indexCount;
};

class VulkanEngine
{
public:
    static const int MAX_FRAMES_IN_FLIGHT = 2;

    VulkanEngine();
    void init();
    void cleanup();

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
    AllocatedHostVisibleAndDeviceLocalBuffer createHostVisibleAndDeviceLocalBuffer(
        size_t allocationSize, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags
    );
    inline void destroyHostVisibleAndDeviceLocalBuffer(
        const AllocatedHostVisibleAndDeviceLocalBuffer& buffer
    ) {
        vmaDestroyBuffer(
            m_allocator, buffer.deviceLocalBuffer.buffer, buffer.deviceLocalBuffer.allocation
        );
        if (!buffer.hostVisibleAndDeviceLocal)
        {
            vmaDestroyBuffer(
                m_allocator, buffer.stagingBuffer.buffer, buffer.deviceLocalBuffer.allocation
            );
        }
    }
    void immediateSubmit(std::function<void(VkCommandBuffer command)>&& function);
    GPUMeshBuffers uploadMesh(std::span<float> vertices, std::span<uint32_t> indices);

    // Images
    AllocatedImage createImage(
        VkExtent3D size, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels = 1,
        VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT
    );
    AllocatedImage createImage(
        void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage,
        uint32_t mipLevels = 1, VkSampleCountFlagBits numMSAAsamples = VK_SAMPLE_COUNT_1_BIT
    );
    void destroyImage(const AllocatedImage& image);

    // Rendering
    void startRenderingFrame(VkExtent2D& swapchainExtent);
    void submitFrame();

private:
    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    bool m_enableValidationLayers;

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
    uint32_t m_currentSwapchainIndex;

    // Rendering
    uint64_t m_currentFrame = 0;
    uint32_t m_frameDataIndex = 0;
    std::vector<FrameData> m_frameData;
    bool m_windowResized = false;

    // Immediate Submit
    VkFence m_immediateSubmitFence;
    VkCommandBuffer m_immediateSubmitCommandBuffer;
    VkCommandPool m_immediateSubmitCommandPool;

    // Device details
    VkPhysicalDeviceProperties2 m_physicalDeviceProperties;
    VkPhysicalDeviceVulkan11Properties m_physicalDeviceVulkan11Properties;
    VkPhysicalDeviceVulkan12Properties m_physicalDeviceVulkan12Properties;
    VkPhysicalDeviceVulkan13Properties m_physicalDeviceVulkan13Properties;
    VkPhysicalDeviceFeatures2 m_physicalDeviceFeatures;
    VkPhysicalDeviceVulkan11Features m_physicalDeviceVulkan11Features;
    VkPhysicalDeviceVulkan12Features m_physicalDeviceVulkan12Features;
    VkPhysicalDeviceVulkan13Features m_physicalDeviceVulkan13Features;
    VkSampleCountFlagBits m_maxMSAAsamples;

    // Initialisation
    bool createInstance();
    void createWindow();
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
    void createSwapchainImageViews();
    void createFrameData();
    void createCommandBuffer(VkCommandPool commandPool, VkCommandBuffer& commandBuffer);
    void createSyncObjects(int frameNum);
    void createAllocator();
    void initImmediateSubmit();

    // Cleanup
    void cleanupSwapchain();
    void cleanupFrameData();
    void cleanupImmediateSubmit();

public:
    inline VkDevice getDevice()
    {
        return m_device;
    }
    inline VkPhysicalDeviceProperties2 getPhysicalDeviceProperties()
    {
        return m_physicalDeviceProperties;
    }
    inline VkPhysicalDeviceFeatures2 getPhysicalDeviceFeatures()
    {
        return m_physicalDeviceFeatures;
    }
    inline VmaAllocator getAllocator()
    {
        return m_allocator;
    }
    inline GLFWwindow* getWindow()
    {
        return m_window;
    }
    inline void singalWindowResize()
    {
        m_windowResized = true;
    }
    inline uint64_t getCurrentFrame()
    {
        return m_currentFrame;
    }
    inline uint32_t getFrameDataIndex()
    {
        return m_frameDataIndex;
    }
    inline FrameData& getCurrentFrameData()
    {
        return m_frameData[m_currentFrame % MAX_FRAMES_IN_FLIGHT];
    }
    inline VkImage getCurrentSwapchainImage()
    {
        return m_swapchainImages[m_currentSwapchainIndex];
    }
    inline VkImageView getCurrentSwapchainImageView()
    {
        return m_swapchainImageViews[m_currentSwapchainIndex];
    }
    inline VkExtent2D getSwapchainExtent()
    {
        return m_swapchainExtent;
    }
    inline VkFormat getSwapchainImageFormat()
    {
        return m_swapchainImageFormat;
    }
    inline VkSampleCountFlagBits getMaxMSAAsamples()
    {
        return m_maxMSAAsamples;
    }
};

}  // namespace lonelycube::client
