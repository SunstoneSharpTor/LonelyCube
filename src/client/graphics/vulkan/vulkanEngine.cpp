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

#include "client/graphics/vulkan/vulkanEngine.h"

#include "client/graphics/vulkan/images.h"
#include "client/graphics/vulkan/pipelines.h"
#include "client/graphics/vulkan/shaders.h"
#include "core/log.h"
#include <vulkan/vulkan_core.h>

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

bool VulkanEngine::init()
{
    initWindow();

    return initVulkan();
}

void VulkanEngine::initWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

bool VulkanEngine::initVulkan()
{
    if (createInstance()
        && createSurface()
        && pickPhysicalDevice()
        && createLogicalDevice()
        && createAllocator()
        && createSwapchain()
        && createSwapchainImageViews()
        && createDrawImage()
        && createFrameData()
        && initDescriptors()
        && initPipelines()
    ) {
        return true;
    }

    return false;
}

void VulkanEngine::cleanupSwapchain()
{
    vkDestroyImageView(m_device, m_drawImage.imageView, nullptr);
    vmaDestroyImage(m_allocator, m_drawImage.image, m_drawImage.allocation);

    for (auto& imageView : m_swapchainImageViews)
        vkDestroyImageView(m_device, imageView, nullptr);

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void VulkanEngine::cleanup()
{
    vkDeviceWaitIdle(m_device);

    cleanupPipelines();

    vkDestroyDescriptorSetLayout(m_device, m_drawImageDescriptorLayout, nullptr);
    globalDescriptorAllocator.destroyPool(m_device);

    cleanupSwapchain();

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
        return false;
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
        LOG("Failed to create instance");
        return false;
    }

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

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
    LOG(deviceProperties.deviceName + std::string(" selected for Vulkan"));

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
    }

    return indices;
}

bool VulkanEngine::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()
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

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
    {
        LOG("Failed to create logical device");
        return false;
    }

    vkGetDeviceQueue(
        m_device, indices.graphicsAndComputeFamily.value(), 0, &m_graphicsAndComputeQueue
    );
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);

    return true;
}

bool VulkanEngine::createSurface()
{
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
    {
        LOG("Failed to create window surface");
        return false;
    }

    return true;
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

bool VulkanEngine::createSwapchain()
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
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
    {
        LOG("Failed to create swap chain");
        return false;
    }

    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;

    return true;
}

bool VulkanEngine::createSwapchainImageViews()
{
    m_swapchainImageViews.resize(m_swapchainImages.size());
    for (size_t i = 0; i < m_swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapchainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[i])
            != VK_SUCCESS)
        {
            LOG("Failed to create image view for swap chain image");
            return false;
        }
    }

    return true;
}

bool VulkanEngine::createFrameData()
{
    m_frameData.resize(m_MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (!(createCommandPool(m_frameData[i].commandPool)
            && createCommandBuffer(m_frameData[i].commandPool, m_frameData[i].commandBuffer)
            && createSyncObjects(i)))
        {
            return false;
        }
    }

    return true;
}

void VulkanEngine::cleanupFrameData()
{
    for (int i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_device, m_frameData[i].imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(m_device, m_frameData[i].renderFinishedSemaphore, nullptr);
        vkDestroyFence(m_device, m_frameData[i].inFlightFence, nullptr);

        vkDestroyCommandPool(m_device, m_frameData[i].commandPool, nullptr);
    }
}

bool VulkanEngine::initImmediateSubmit()
{
    // if (

    return true;
}

bool VulkanEngine::createCommandPool(VkCommandPool& commandPool)
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        LOG("Failed to create command pool");
        return false;
    }

    return true;
}

bool VulkanEngine::createCommandBuffer(VkCommandPool commandPool, VkCommandBuffer& commandBuffer)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer) != VK_SUCCESS)
    {
        LOG("Failed to allocate command buffer");
        return false;
    }

    return true;
}

bool VulkanEngine::createSyncObjects(int frameNum)
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(
        m_device, &semaphoreInfo, nullptr, &m_frameData[frameNum].imageAvailableSemaphore)
        != VK_SUCCESS || vkCreateSemaphore(
        m_device, &semaphoreInfo, nullptr, &m_frameData[frameNum].renderFinishedSemaphore)
        != VK_SUCCESS)
    {
        LOG("Failed to create semaphores");
        return false;
    }

    if (vkCreateFence(m_device, &fenceInfo, nullptr, &m_frameData[frameNum].inFlightFence)
        != VK_SUCCESS)
    {
        LOG("Failed to create fence");
        return false;
    }

    return true;
}

void VulkanEngine::drawBackgroud(VkCommandBuffer command)
{
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradientPipeline);
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradientPipelineLayout,
        0, 1, &m_drawImageDescriptors,
        0, nullptr
    );
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    x /= m_swapchainExtent.width;
    y /= m_swapchainExtent.height;
    glm::vec4 colours[2] = { { x, 1 - x, y, 1 }, { 1 - y, y, 1 - x, 1 } };
    vkCmdPushConstants(
        command, m_gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 2 * sizeof(glm::vec4),
        colours
    );
    vkCmdDispatch(
        command, (m_swapchainExtent.width + 15) / 16, (m_swapchainExtent.height + 15) / 16, 1
    );
}

void VulkanEngine::drawGeometry(VkCommandBuffer command)
{
    VkRenderingAttachmentInfo colourAttachment{};
    colourAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colourAttachment.imageView = m_drawImage.imageView;
    colourAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = { { 0, 0 }, m_swapchainExtent };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colourAttachment;
    renderingInfo.pDepthAttachment = nullptr;

    vkCmdBeginRendering(command, &renderingInfo);
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_trianglePipeline);

    VkViewport viewport{};
    viewport.width = m_swapchainExtent.width;
    viewport.height = m_swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent.width = m_swapchainExtent.width;
    scissor.extent.height = m_swapchainExtent.height;

    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdSetScissor(command, 0, 1, &scissor);
    vkCmdDraw(command, 3, 1, 0, 0);
    vkCmdEndRendering(command);
}

bool VulkanEngine::recordCommandBuffer(
    VkCommandBuffer command, uint32_t swapchainImageIndex
) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command, &beginInfo) != VK_SUCCESS)
    {
        LOG("Failed to begin recording command buffer");
        return false;
    }

    transitionImage(command, m_drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    drawBackgroud(command);
    transitionImage(
        command, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    drawGeometry(command);

    transitionImage(
        command, m_drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );
    transitionImage(
        command, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    blitImageToImage(
        command, m_drawImage.image, m_swapchainImages[swapchainImageIndex], m_swapchainExtent,
        m_swapchainExtent, VK_FILTER_NEAREST
    );
    transitionImage(
        command, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );

    if (vkEndCommandBuffer(command) != VK_SUCCESS)
    {
        LOG("Failed to record command buffer");
        return false;
    }

    return true;
}

bool VulkanEngine::drawFrame()
{
    FrameData currentFrameData = m_frameData[m_currentFrame % 2];

    vkWaitForFences(m_device, 1, &currentFrameData.inFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t swapchainImageIndex;
    VkResult result = vkAcquireNextImageKHR(
        m_device, m_swapchain, UINT64_MAX, currentFrameData.imageAvailableSemaphore,
        VK_NULL_HANDLE, &swapchainImageIndex
    );

    // Check for resizing
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return true;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        LOG("Failed to acquire swap chain image");
        return false;
    }

    vkResetFences(m_device, 1, &currentFrameData.inFlightFence);

    VkCommandBuffer commandBuffer = currentFrameData.commandBuffer;
    vkResetCommandBuffer(commandBuffer, 0);
    recordCommandBuffer(commandBuffer, swapchainImageIndex);

    VkCommandBufferSubmitInfo commandSubmitInfo{};
    commandSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandSubmitInfo.commandBuffer = commandBuffer;

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

    if (vkQueueSubmit2(m_graphicsAndComputeQueue, 1, &submitInfo, currentFrameData.inFlightFence)
        != VK_SUCCESS)
    {
        LOG("Failed to submit draw command buffer");
        return false;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &currentFrameData.renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &swapchainImageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

    // Check for resizing
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_windowResized)
    {
        m_windowResized = false;
        recreateSwapchain();
    }
    else if (result != VK_SUCCESS)
    {
        LOG("Failed to present swap chain image");
        return false;
    }

    m_currentFrame++;

    return true;
}

bool VulkanEngine::recreateSwapchain()
{
    vkDeviceWaitIdle(m_device);

    cleanupSwapchain();

    if(createSwapchain()
        && createSwapchainImageViews()
        && createDrawImage())
    {
        updateDrawImageDescriptor();

        return true;
    }

    return false;
}

bool VulkanEngine::createAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
    {
        LOG("Failed to create VMAallocator");
        return false;
    }

    return true;
}

bool VulkanEngine::createDrawImage()
{
    VkExtent3D drawImageExtent = {
        m_swapchainExtent.width,
        m_swapchainExtent.height,
        1
    };

    m_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    m_drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
        | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        | VK_IMAGE_USAGE_STORAGE_BIT
        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = m_drawImage.imageFormat;
    imageCreateInfo.extent = m_drawImage.imageExtent;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = drawImageUsages;

    VmaAllocationCreateInfo imageAllocInfo{};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vmaCreateImage(
        m_allocator, &imageCreateInfo, &imageAllocInfo, &m_drawImage.image, &m_drawImage.allocation,
        nullptr) != VK_SUCCESS)
    {
        LOG("Failed to create draw image");
        return false;
    }

    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.image = m_drawImage.image;
    imageViewCreateInfo.format = m_drawImage.imageFormat;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &m_drawImage.imageView)
        != VK_SUCCESS)
    {
        LOG("Failed to create image view for draw image");
        return false;
    }

    return true;
}

void VulkanEngine::updateDrawImageDescriptor()
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = m_drawImage.imageView;

    VkWriteDescriptorSet drawImageWrite{};
    drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    drawImageWrite.dstBinding = 0;
    drawImageWrite.dstSet = m_drawImageDescriptors;
    drawImageWrite.descriptorCount = 1;
    drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    drawImageWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device, 1, &drawImageWrite, 0, nullptr);
}

bool VulkanEngine::initDescriptors()
{
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
    {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
    };

    if (!globalDescriptorAllocator.initPool(m_device, 10, sizes))
        return false;

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        m_drawImageDescriptorLayout = builder.build(m_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    m_drawImageDescriptors = globalDescriptorAllocator.allocate(
        m_device, m_drawImageDescriptorLayout
    );

    updateDrawImageDescriptor();

    return true;
}

bool VulkanEngine::initBackgroundPipelines()
{
    VkPipelineLayoutCreateInfo computePipelineLayout{};
    computePipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computePipelineLayout.setLayoutCount = 1;
    computePipelineLayout.pSetLayouts = &m_drawImageDescriptorLayout;

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = 2 * sizeof(glm::vec4);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    computePipelineLayout.pushConstantRangeCount = 1;
    computePipelineLayout.pPushConstantRanges = &pushConstant;

    if (vkCreatePipelineLayout(m_device, &computePipelineLayout, nullptr, &m_gradientPipelineLayout)
        != VK_SUCCESS)
    {
        LOG("Failed to create draw image blit pipeline layout");
        return false;
    }

    VkShaderModule computeDrawShader;
    if (!createShaderModule(m_device, "res/shaders/gradient.comp.spv", computeDrawShader))
        return false;

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = computeDrawShader;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.layout = m_gradientPipelineLayout;
    computePipelineCreateInfo.stage = stageInfo;

    if (vkCreateComputePipelines(
        m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_gradientPipeline)
        != VK_SUCCESS)
    {
        LOG("Failed to create gradient compute pipeline");
        return false;
    }

    vkDestroyShaderModule(m_device, computeDrawShader, nullptr);

    return true;
}

void VulkanEngine::cleanupBackgroundPipelines()
{
    vkDestroyPipeline(m_device, m_gradientPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_gradientPipelineLayout, nullptr);
}

bool VulkanEngine::initTrianglePipeline()
{
    VkShaderModule triangleVertexShader;
    if (!createShaderModule(m_device, "res/shaders/test.vert.spv", triangleVertexShader))
        return false;

    VkShaderModule triangleFragmentShader;
    if (!createShaderModule(m_device, "res/shaders/test.frag.spv", triangleFragmentShader))
        return false;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_trianglePipelineLayout)
        != VK_SUCCESS)
    {
        LOG("Failed to create graphics pipeline layout");
        return false;
    }

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.pipelineLayout = m_trianglePipelineLayout;
    pipelineBuilder.setShaders(triangleVertexShader, triangleFragmentShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.disableBlending();
    pipelineBuilder.disableDepthTest();
    pipelineBuilder.setColourAttachmentFormat(m_drawImage.imageFormat);
    pipelineBuilder.setDepthAttachmentFormat(VK_FORMAT_UNDEFINED);

    m_trianglePipeline = pipelineBuilder.buildPipeline(m_device);

    if (m_trianglePipeline == VK_NULL_HANDLE)
        return false;

    vkDestroyShaderModule(m_device, triangleVertexShader, nullptr);
    vkDestroyShaderModule(m_device, triangleFragmentShader, nullptr);

    return true;
}

void VulkanEngine::cleanupTrianglePipeline()
{
    vkDestroyPipelineLayout(m_device, m_trianglePipelineLayout, nullptr);
    vkDestroyPipeline(m_device, m_trianglePipeline, nullptr);
}

bool VulkanEngine::initPipelines()
{
    return initBackgroundPipelines() && initTrianglePipeline();
}

void VulkanEngine::cleanupPipelines()
{
    cleanupBackgroundPipelines();
    cleanupTrianglePipeline();
}

AllocatedBuffer VulkanEngine::createBuffer(size_t allocationSize, VkBufferUsageFlags usage) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = allocationSize;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    AllocatedBuffer newBuffer;
    if (vmaCreateBuffer(
        m_allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation,
        &newBuffer.info) != VK_SUCCESS)
    {
        LOG("Failed to allocate buffer");
    }

    return newBuffer;
}

// GPUMeshBuffers VulkanEngine::uploadMesh(std::span<float> vertices, std::span<uint32_t> indices)
// {
//     const size_t vertexBufferSize = vertices.size() * sizeof(float);
//     const size_t indexBufferSize = indices.size() * sizeof(uint32_t);
//
//     GPUMeshBuffers newMesh;
//     newMesh.vertexBuffer = createBuffer(
//         vertexBufferSize,
//         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
//         | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
//     );
//
//     VkBufferDeviceAddressInfo deviceAddressInfo{};
//     deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
//     deviceAddressInfo.buffer = newMesh.vertexBuffer.buffer;
//
//     newMesh.vertexBufferAddress = vkGetBufferDeviceAddress(m_device, &deviceAddressInfo);
//
//     newMesh.indexBuffer = createBuffer(
//         indexBufferSize,
//         VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
//     );
//
//     AllocatedBuffer staging = createBuffer(
//         vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
//     );
//
//     void* data = staging.allocation->GetMappedData();
//
//     memcpy(data, vertices.data(), vertexBufferSize);
//     memcpy(reinterpret_cast<char*>(data) + vertexBufferSize, indices.data(), indexBufferSize);
//
//     immediateSubmit([&](VkCommandBuffer command) {
//         VkBufferCopy vertexCopy{};
//         vertexCopy.size = vertexBufferSize;
//
//         vkCmdCopyBuffer(command, staging.buffer, newMesh.vertexBuffer.buffer, 1, &vertexCopy);
//     });
// }

}  // namespace lonelycube::lient
