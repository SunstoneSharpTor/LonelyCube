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

#include "core/pch.h"

namespace lonelycube::client {

class HelloTriangleApplication
{
public:
    HelloTriangleApplication();
    void run();

private:
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
    };

    const uint32_t m_WIDTH = 800;
    const uint32_t m_HEIGHT = 600;

    GLFWwindow* m_window;
    VkInstance m_instance;
    const bool m_enableValidationLayers;
    const std::vector<const char*> s_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

    void initWindow();
    bool initVulkan();
    void mainLoop();
    void cleanup();

    bool createInstance();
    bool checkValidationLayerSupport();
    bool pickPhysicalDevice();
    int ratePhysicalDeviceSuitability(const VkPhysicalDevice& device);
    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device);
};

}  // namespace lonelycube::client
