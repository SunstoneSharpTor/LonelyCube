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

#include "client/graphics/vulkan/vulkanEngine.h"

#include "client/graphics/vulkan/images.h"
#include "client/graphics/vulkan/utils.h"
#include "core/log.h"
#include <cassert>
#include <string>
#include <vulkan/vulkan_core.h>

#include "glm/glm.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE STBIR_FILTER_BOX
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace lonelycube::client {

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    app->singalWindowResize();
}

VulkanEngine::VulkanEngine() :
#ifdef RELEASE
    m_enableValidationLayers(false)
#else
    m_enableValidationLayers(true)
#endif
{}

void VulkanEngine::init()
{
    int glfwInitialized = glfwInit();
    assert(glfwInitialized == GLFW_TRUE && "Could not initialise GLFW");
    assert(glfwVulkanSupported() == GLFW_TRUE && "GLFW: Vulkan not supported");
    VkResult volkInitialized = volkInitialize();
    assert(volkInitialized == VK_SUCCESS && "Vulkan loader not installed");

    createWindow();
    bool instanceCreated = createInstance();
    assert(instanceCreated);
    glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
    pickPhysicalDevice();
    createLogicalDevice();
    createAllocator();
    createSwapchain();
    createSwapchainImageViews();
    createFrameData();
    initImmediateSubmit();
}

void VulkanEngine::createWindow()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    const GLFWvidmode* displayMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int windowDimensions[2] = { displayMode->width / 2, displayMode->height / 2 };
    m_window = glfwCreateWindow(
        windowDimensions[0], windowDimensions[1], "Lonely Cube", nullptr, nullptr
    );

    glfwSetWindowPos(
        m_window,
        displayMode->width / 2 - windowDimensions[0] / 2,
        displayMode->height / 2 - windowDimensions[1] / 2
    );

    GLFWimage images[1];
    images[0].pixels = stbi_load(
        "res/resourcePack/logo.png", &images[0].width, &images[0].height, 0, 4
    );
    glfwSetWindowIcon(m_window, 1, images);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void VulkanEngine::cleanupSwapchain()
{
    for (auto& imageView : m_swapchainImageViews)
        vkDestroyImageView(m_device, imageView, nullptr);

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void VulkanEngine::cleanup()
{
    vkDeviceWaitIdle(m_device);

    cleanupSwapchain();

    cleanupImmediateSubmit();

    cleanupFrameData();

    vmaDestroyAllocator(m_allocator);

    vkDestroyDevice(m_device, nullptr);

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    glfwDestroyWindow(m_window);

    glfwTerminate();
}

bool VulkanEngine::createInstance()
{
    if (m_enableValidationLayers && !checkValidationLayerSupport())
    {
        LOG("Validation layers requested but not available");
        m_enableValidationLayers = false;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Lonely Cube";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    if (m_enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
    {
        LOG("Failed to create vulkan instance");
        return false;
    }

    volkLoadInstance(m_instance);

    return true;
}

bool VulkanEngine::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName: m_validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
            return false;
    }

    return true;
}

bool VulkanEngine::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        LOG("Failed to find devices with vulkan support");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    int maxRating = 0;
    for (const auto& device : devices)
    {
        int rating = ratePhysicalDeviceSuitability(device);
        if (rating > maxRating)
        {
            m_physicalDevice = device;
            maxRating = rating;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        LOG("Failed to find a suitable GPU");
        return false;
    }

    m_physicalDeviceVulkan13Properties.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
    m_physicalDeviceVulkan13Properties.pNext = nullptr;
    m_physicalDeviceVulkan12Properties.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
    m_physicalDeviceVulkan12Properties.pNext = &m_physicalDeviceVulkan13Properties;
    m_physicalDeviceVulkan11Properties.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
    m_physicalDeviceVulkan11Properties.pNext = &m_physicalDeviceVulkan12Properties;
    m_physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    m_physicalDeviceProperties.pNext = &m_physicalDeviceVulkan11Properties;
    vkGetPhysicalDeviceProperties2(m_physicalDevice, &m_physicalDeviceProperties);

    LOG(m_physicalDeviceProperties.properties.deviceName + std::string(" selected for Vulkan"));

    m_physicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    m_physicalDeviceVulkan13Features.pNext = nullptr;
    m_physicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    m_physicalDeviceVulkan12Features.pNext = &m_physicalDeviceVulkan13Features;
    m_physicalDeviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    m_physicalDeviceVulkan11Features.pNext = &m_physicalDeviceVulkan12Features;
    m_physicalDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_physicalDeviceFeatures.pNext = &m_physicalDeviceVulkan11Features;
    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &m_physicalDeviceFeatures);

    VkSampleCountFlags counts =
        m_physicalDeviceProperties.properties.limits.framebufferColorSampleCounts
        & m_physicalDeviceProperties.properties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) { m_maxMSAAsamples = VK_SAMPLE_COUNT_64_BIT; }
    else if (counts & VK_SAMPLE_COUNT_32_BIT) { m_maxMSAAsamples = VK_SAMPLE_COUNT_32_BIT; }
    else if (counts & VK_SAMPLE_COUNT_16_BIT) { m_maxMSAAsamples = VK_SAMPLE_COUNT_16_BIT; }
    else if (counts & VK_SAMPLE_COUNT_8_BIT) { m_maxMSAAsamples = VK_SAMPLE_COUNT_8_BIT; }
    else if (counts & VK_SAMPLE_COUNT_4_BIT) { m_maxMSAAsamples = VK_SAMPLE_COUNT_4_BIT; }
    else if (counts & VK_SAMPLE_COUNT_2_BIT) { m_maxMSAAsamples = VK_SAMPLE_COUNT_2_BIT; }
    else { m_maxMSAAsamples = VK_SAMPLE_COUNT_1_BIT; }

    return true;
}

int VulkanEngine::ratePhysicalDeviceSuitability(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    if (VK_VERSION_MINOR(deviceProperties.apiVersion) < 3
        && VK_VERSION_MAJOR(deviceProperties.apiVersion) <= 1
    ) {
        return 0;
    }

    QueueFamilyIndices indices = findQueueFamilies(device);

    if (!(indices.graphicsAndComputeFamily.has_value() && indices.presentFamily.has_value()))
        return 0;

    if (!checkDeviceExtensionSupport(device))
        return 0;

    if (!checkDeviceFeaturesSupport(device))
        return 0;

    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device);
    if (swapchainSupport.formats.empty() || swapchainSupport.presentModes.empty())
        return 0;

    int score = 1;
    score += 300 * (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
    score += 1000 * (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

    return score;
}

bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &extensionCount, availableExtensions.data()
    );

    std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

    for (const auto& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

bool VulkanEngine::checkDeviceFeaturesSupport(VkPhysicalDevice device)
{
    VkPhysicalDeviceVulkan13Features device13features{};
    device13features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    VkPhysicalDeviceVulkan12Features device12features{};
    device12features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    device12features.pNext = &device13features;

    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &device12features;

    vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

    return device12features.bufferDeviceAddress == VK_TRUE
        && device12features.descriptorIndexing == VK_TRUE
        && device13features.dynamicRendering == VK_TRUE
        && device13features.synchronization2 == VK_TRUE;
}

SwapchainSupportDetails VulkanEngine::querySwapchainSupport(
    VkPhysicalDevice device
) {
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, m_surface, &formatCount, details.formats.data()
        );
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, m_surface, &presentModeCount, details.presentModes.data()
        );
    }

    return details;
}

QueueFamilyIndices VulkanEngine::findQueueFamilies(VkPhysicalDevice device)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    QueueFamilyIndices indices;
    int i = 0;
    for (int i = 0; i < queueFamilies.size(); i++)
    {
        // Make the graphicsAndComputeFamily favour the first family that also supports presentation
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT
            && queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT
            && indices.graphicsAndComputeFamily.value_or(i)
                != indices.presentFamily.value_or(i + 1))
        {
            indices.graphicsAndComputeFamily = i;
        }

        // Make the compute family favour a family other than the graphicsAndComputeFamily
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT
            && (!indices.computeFamily.has_value()
                || indices.graphicsAndComputeFamily.value_or(i + 1) != i))
        {
            indices.computeFamily = i;
        }

        // Try to make the present family the same as the graphicsAndComputeFamily
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if (presentSupport
            && (!indices.presentFamily.has_value()
                || indices.graphicsAndComputeFamily.value_or(i + 1) == i))
        {
            indices.presentFamily = i;
        }

        // Make the transfer queue family favour a family that is transfer only
        if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT
            && (!indices.transferFamily.has_value()
                || !(queueFamilies[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT
                    | VK_QUEUE_VIDEO_DECODE_BIT_KHR))))
        {
            indices.transferFamily = i;
        }

        // LOG(std::to_string(i) + ": "
        //     + (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ? "graphics " : "")
        //     + (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT ? "compute " : "")
        //     + (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT ? "transfer " : "")
        //     + (queueFamilies[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR ? "decode " : "")
        //     + (presentSupport ? "present" : "")
        // );
    }

    return indices;
}

void VulkanEngine::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsAndComputeFamily.value(),
        indices.presentFamily.value(),
        indices.transferFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceVulkan13Features device13features{};
    device13features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    device13features.dynamicRendering = VK_TRUE;
    device13features.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features device12features{};
    device12features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    device12features.pNext = &device13features;
    device12features.bufferDeviceAddress = VK_TRUE;
    device12features.descriptorIndexing = VK_TRUE;

    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &device12features;
    deviceFeatures.features.samplerAnisotropy = m_physicalDeviceFeatures.features.samplerAnisotropy;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &deviceFeatures;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

    if (m_enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VK_CHECK(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device));

    vkGetDeviceQueue(
        m_device, indices.graphicsAndComputeFamily.value(), 0, &m_graphicsAndComputeQueue
    );
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
    vkGetDeviceQueue(m_device, indices.transferFamily.value(), 0, &m_transferQueue);

    // LOG("Graphics and Compute: " + std::to_string(indices.graphicsAndComputeFamily.value_or(-1))
    //     + ", Compute: " + std::to_string(indices.computeFamily.value_or(-1))
    //     + ", Transfer: " + std::to_string(indices.transferFamily.value_or(-1))
    //     + ", Present: " + std::to_string(indices.presentFamily.value_or(-1))
    // );
}

VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats
) {
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
            && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        )
            return availableFormat;
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanEngine::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes
) {
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }

    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            return availablePresentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanEngine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

        actualExtent.width = std::clamp(
            actualExtent.width,
            capabilities.minImageExtent.width, capabilities.maxImageExtent.width
        );
        actualExtent.height = std::clamp(
            actualExtent.height,
            capabilities.minImageExtent.height, capabilities.maxImageExtent.height
        );

        return actualExtent;
    }
}

void VulkanEngine::createSwapchain()
{
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0
        && imageCount > swapchainSupport.capabilities.maxImageCount
    ) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {
        indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()
    };

    if (indices.graphicsAndComputeFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
        LOG("Graphics and presentation queue families differ, currently causes worse performance");
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain));

    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;
}

void VulkanEngine::createSwapchainImageViews()
{
    m_swapchainImageViews.resize(m_swapchainImages.size());
    for (size_t i = 0; i < m_swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapchainImageFormat;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[i]));
    }
}

void VulkanEngine::createCommandBuffer(VkCommandPool commandPool, VkCommandBuffer& commandBuffer)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer));
}

void VulkanEngine::createFrameData()
{
    m_frameData.resize(MAX_FRAMES_IN_FLIGHT);
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

        VK_CHECK(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_frameData[i].commandPool));

        createCommandBuffer(m_frameData[i].commandPool, m_frameData[i].commandBuffer);
        createSyncObjects(i);
    }
}

void VulkanEngine::cleanupFrameData()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_device, m_frameData[i].imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(m_device, m_frameData[i].renderFinishedSemaphore, nullptr);
        vkDestroyFence(m_device, m_frameData[i].inFlightFence, nullptr);

        vkDestroyCommandPool(m_device, m_frameData[i].commandPool, nullptr);
    }
}

void VulkanEngine::initImmediateSubmit()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();

    VK_CHECK(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_immediateSubmitCommandPool));

    createCommandBuffer(m_immediateSubmitCommandPool, m_immediateSubmitCommandBuffer);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_immediateSubmitFence));
}

void VulkanEngine::cleanupImmediateSubmit()
{
    vkDestroyCommandPool(m_device, m_immediateSubmitCommandPool, nullptr);
    vkDestroyFence(m_device, m_immediateSubmitFence, nullptr);
}

void VulkanEngine::immediateSubmit(std::function<void(VkCommandBuffer command)>&& function)
{
    VK_CHECK(vkResetFences(m_device, 1, &m_immediateSubmitFence));
    VK_CHECK(vkResetCommandBuffer(m_immediateSubmitCommandBuffer, 0));

    VkCommandBuffer command = m_immediateSubmitCommandBuffer;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    VK_CHECK(vkBeginCommandBuffer(command, &beginInfo));

    function(command);

    VK_CHECK(vkEndCommandBuffer(command));

    VkCommandBufferSubmitInfo commandSubmitInfo{};
    commandSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandSubmitInfo.commandBuffer = command;

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandSubmitInfo;

    VK_CHECK(vkQueueSubmit2(m_transferQueue, 1, &submitInfo, m_immediateSubmitFence));

    VK_CHECK(vkWaitForFences(m_device, 1, &m_immediateSubmitFence, true, UINT64_MAX));
}

void VulkanEngine::createSyncObjects(int frameNum)
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_CHECK(vkCreateSemaphore(
        m_device, &semaphoreInfo, nullptr, &m_frameData[frameNum].imageAvailableSemaphore)
    );
    VK_CHECK(vkCreateSemaphore(
        m_device, &semaphoreInfo, nullptr, &m_frameData[frameNum].renderFinishedSemaphore)
    );

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_frameData[frameNum].inFlightFence));
}

void VulkanEngine::startRenderingFrame(VkExtent2D& swapchainExtent)
{
    FrameData& currentFrameData = m_frameData[m_frameDataIndex];

    VK_CHECK(vkWaitForFences(m_device, 1, &currentFrameData.inFlightFence, VK_TRUE, UINT64_MAX));

    VkResult result = vkAcquireNextImageKHR(
        m_device, m_swapchain, UINT64_MAX, currentFrameData.imageAvailableSemaphore,
        VK_NULL_HANDLE, &m_currentSwapchainIndex
    );

    // Check for resizing
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
    }
    else if (result != VK_SUBOPTIMAL_KHR)
    {
        assert(result == VK_SUCCESS);
    }

    VK_CHECK(vkResetFences(m_device, 1, &currentFrameData.inFlightFence));

    VkCommandBuffer commandBuffer = currentFrameData.commandBuffer;
    VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

    swapchainExtent = m_swapchainExtent;
}

void VulkanEngine::submitFrame()
{
    FrameData& currentFrameData = m_frameData[m_frameDataIndex];

    VkCommandBufferSubmitInfo commandSubmitInfo{};
    commandSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandSubmitInfo.commandBuffer = currentFrameData.commandBuffer;

    VkSemaphoreSubmitInfo waitInfo{};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo.semaphore = currentFrameData.imageAvailableSemaphore;
    waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    waitInfo.value = 1;

    VkSemaphoreSubmitInfo signalInfo{};
    signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo.semaphore = currentFrameData.renderFinishedSemaphore;
    signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    signalInfo.value = 1;

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandSubmitInfo;

    VK_CHECK(
        vkQueueSubmit2(m_graphicsAndComputeQueue, 1, &submitInfo, currentFrameData.inFlightFence)
    );

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &currentFrameData.renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_currentSwapchainIndex;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

    // Check for resizing
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_windowResized)
    {
        m_windowResized = false;
        recreateSwapchain();
    }
    else
    {
        assert(result == VK_SUCCESS);
    }

    m_currentFrame++;
    m_frameDataIndex = m_currentFrame % MAX_FRAMES_IN_FLIGHT;
}

void VulkanEngine::recreateSwapchain()
{
    vkDeviceWaitIdle(m_device);

    cleanupSwapchain();

    createSwapchain();
    createSwapchainImageViews();
}

void VulkanEngine::createAllocator()
{
    VmaVulkanFunctions vmaVulkanFunctions{};
    vmaVulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vmaVulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_instance;
    allocatorInfo.pVulkanFunctions = &vmaVulkanFunctions;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_allocator));
}

AllocatedBuffer VulkanEngine::createBuffer(
    size_t allocationSize, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags
) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = allocationSize;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaAllocInfo.flags = flags;

    AllocatedBuffer newBuffer;
    VK_CHECK(vmaCreateBuffer(
        m_allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation,
        &newBuffer.info
    ));

    return newBuffer;
}

AllocatedHostVisibleAndDeviceLocalBuffer VulkanEngine::createHostVisibleAndDeviceLocalBuffer(
    size_t allocationSize, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags
) {
    AllocatedHostVisibleAndDeviceLocalBuffer newBuffer;
    VmaAllocationCreateFlags hostVisibleAndDeviceLocalFlags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT;
    newBuffer.deviceLocalBuffer = createBuffer(
        allocationSize, usage, hostVisibleAndDeviceLocalFlags | flags
    );

    VkMemoryPropertyFlags memPropFlags;
    vmaGetAllocationMemoryProperties(
        m_allocator, newBuffer.deviceLocalBuffer.allocation, &memPropFlags
    );
    newBuffer.hostVisibleAndDeviceLocal = memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    if (newBuffer.hostVisibleAndDeviceLocal)
    {
        newBuffer.mappedData = newBuffer.deviceLocalBuffer.info.pMappedData;
    }
    else
    {
        newBuffer.stagingBuffer = createBuffer(
            allocationSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT 
        );
        newBuffer.mappedData = newBuffer.stagingBuffer.info.pMappedData;
    }

    return newBuffer;
}

void VulkanEngine::updateHostVisibleAndDeviceLocalBuffer(
    VkCommandBuffer command, AllocatedHostVisibleAndDeviceLocalBuffer& buffer, uint32_t size,
    VkAccessFlags accessMask, VkPipelineStageFlagBits dstStageMask
) {
    VK_CHECK(vmaFlushAllocation(m_allocator, buffer.deviceLocalBuffer.allocation, 0, size));

    VkBufferMemoryBarrier bufMemBarrier{};
    bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufMemBarrier.dstAccessMask = accessMask;
    bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.buffer = buffer.deviceLocalBuffer.buffer;
    bufMemBarrier.offset = 0;
    bufMemBarrier.size = size;

    if (buffer.hostVisibleAndDeviceLocal)
    {
        bufMemBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        vkCmdPipelineBarrier(
            command, VK_PIPELINE_STAGE_HOST_BIT, dstStageMask, 0, 0, nullptr, 1, &bufMemBarrier, 0,
            nullptr
        );
    }
    else
    {
        VkBufferMemoryBarrier bufMemBarrier2{};
        bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufMemBarrier2.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        bufMemBarrier2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        bufMemBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier2.buffer = buffer.stagingBuffer.buffer;
        bufMemBarrier2.offset = 0;
        bufMemBarrier2.size = size;
        vkCmdPipelineBarrier(
            command, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
            &bufMemBarrier2, 0, nullptr
        );

        VkBufferCopy bufferCopy = {
            0,  // srcOffset
            0,  // dstOffset,
            size,  // size
        };
        vkCmdCopyBuffer(
            command, buffer.stagingBuffer.buffer, buffer.deviceLocalBuffer.buffer, 1, &bufferCopy
        );

        bufMemBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(
            command, VK_PIPELINE_STAGE_TRANSFER_BIT, dstStageMask, 0, 0, nullptr, 1,
            &bufMemBarrier, 0, nullptr
        );
    }
}

GPUMeshBuffers VulkanEngine::uploadMesh(std::span<float> vertices, std::span<uint32_t> indices)
{
    const size_t vertexBufferSize = vertices.size() * sizeof(float);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers newMesh;
    newMesh.vertexBuffer = createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        0
    );

    VkBufferDeviceAddressInfo deviceAddressInfo{};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer = newMesh.vertexBuffer.buffer;
    newMesh.vertexBufferAddress = vkGetBufferDeviceAddress(m_device, &deviceAddressInfo);

    newMesh.indexBuffer = createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        0
    );

    AllocatedBuffer staging = createBuffer(
        vertexBufferSize + indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT 
    );

    memcpy(staging.info.pMappedData, vertices.data(), vertexBufferSize);
    memcpy(
        reinterpret_cast<int8_t*>(staging.info.pMappedData) + vertexBufferSize, indices.data(),
        indexBufferSize
    );

    immediateSubmit([&](VkCommandBuffer command) {
        VkBufferCopy vertexCopy{};
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(command, staging.buffer, newMesh.vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{};
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(command, staging.buffer, newMesh.indexBuffer.buffer, 1, &indexCopy);
    });

    destroyBuffer(staging);

    newMesh.indexCount = indices.size();

    return newMesh;
}

GPUDynamicMeshBuffers VulkanEngine::allocateDynamicMesh(
    uint32_t maxVertexBufferSize, uint32_t maxIndexBufferSize
) {
    GPUDynamicMeshBuffers newMesh;
    newMesh.vertexBuffer = createHostVisibleAndDeviceLocalBuffer(
        maxVertexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        0
    );

    VkBufferDeviceAddressInfo deviceAddressInfo{};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer = newMesh.vertexBuffer.deviceLocalBuffer.buffer;
    newMesh.vertexBufferAddress = vkGetBufferDeviceAddress(m_device, &deviceAddressInfo);

    newMesh.indexBuffer = createHostVisibleAndDeviceLocalBuffer(
        maxIndexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        0
    );

    newMesh.indexCount = 0;

    return newMesh;
}

void VulkanEngine::updateDynamicMesh(
    VkCommandBuffer command, GPUDynamicMeshBuffers& mesh, uint32_t vertexBufferSize,
    uint32_t indexCount
) {
    if (vertexBufferSize > 0)
    {
        LOG(std::to_string(vertexBufferSize));
        updateHostVisibleAndDeviceLocalBuffer(
            command, mesh.vertexBuffer, vertexBufferSize, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        );
    }
    if (indexCount > 0)
    {
        LOG(std::to_string(indexCount));
        updateHostVisibleAndDeviceLocalBuffer(
            command, mesh.indexBuffer, indexCount * sizeof(float), VK_ACCESS_INDEX_READ_BIT,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
        );
    }
    mesh.indexCount = indexCount;
}

AllocatedImage VulkanEngine::createImage(
    VkExtent3D size, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels,
    VkSampleCountFlagBits numSamples
) {
    AllocatedImage newImage;
    newImage.imageFormat = format;
    newImage.imageExtent = size;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent = size;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = numSamples;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = usage;

    VmaAllocationCreateInfo imageAllocInfo{};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    imageAllocInfo.flags = 0;

    VK_CHECK(vmaCreateImage(
        m_allocator, &imageCreateInfo, &imageAllocInfo, &newImage.image, &newImage.allocation,
        nullptr
    ));

    VkImageAspectFlags aspectFlag = format == VK_FORMAT_D32_SFLOAT ?
        VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.image = newImage.image;
    imageViewCreateInfo.format = newImage.imageFormat;
    imageViewCreateInfo.subresourceRange.aspectMask = aspectFlag;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &newImage.imageView));

    return newImage;
}

AllocatedImage VulkanEngine::createImage(
    void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels,
    VkSampleCountFlagBits numMSAAsamples
) {
    AllocatedImage newImage = createImage(
        size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        mipLevels, numMSAAsamples
    );

    std::vector<AllocatedBuffer> stagingBuffers(mipLevels);

    immediateSubmit([&](VkCommandBuffer command) {
        for (int mipLevel = 0; mipLevel < mipLevels; mipLevel++)
        {
            VkExtent3D halfSize;
            if (mipLevel > 0)
                halfSize = VkExtent3D(
                    size.width > 3 ? size.width / 2 : 1, size.height > 3 ? size.height / 2 : 1,
                    size.depth
                );
            else
                halfSize = size;

            size_t halfDataSize = halfSize.depth * halfSize.width * halfSize.height * 4;
            stagingBuffers[mipLevel] = createBuffer(
                halfDataSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                    | VMA_ALLOCATION_CREATE_MAPPED_BIT
            );

            if (mipLevel == 0)
            {
                memcpy(stagingBuffers[mipLevel].info.pMappedData, data, halfDataSize);
                transitionImage(
                    command, newImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                );
            }
            else
            {
                stbir_resize_uint8_srgb(
                    static_cast<uint8_t*>(stagingBuffers[mipLevel - 1].info.pMappedData), size.width,
                    size.height, 0, static_cast<uint8_t*>(stagingBuffers[mipLevel].info.pMappedData),
                    halfSize.width, halfSize.height, 0, stbir_pixel_layout::STBIR_RGBA
                );
            }

            VkBufferImageCopy copyRegion{};
            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = mipLevel;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = halfSize;

            vkCmdCopyBufferToImage(
                command, stagingBuffers[mipLevel].buffer, newImage.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion
            );

            if (mipLevel == mipLevels - 1)
                transitionImage(
                    command, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                );
            else if (mipLevel > 0)
                size = halfSize;
        }
    });

    for (const auto& buffer : stagingBuffers)
        destroyBuffer(buffer);

    return newImage;
}

void VulkanEngine::destroyImage(const AllocatedImage& image)
{
    vkDestroyImageView(m_device, image.imageView, nullptr);
    vmaDestroyImage(m_allocator, image.image, image.allocation);
}

}  // namespace lonelycube::lient
