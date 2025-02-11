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

#include "client/graphics/vulkan/shaders.h"

#include "client/graphics/vulkan/utils.h"
#include "core/log.h"

namespace lonelycube::client {

bool createShaderModule(
    VkDevice device, const std::string& srcFileName, VkShaderModule& shaderModule
) {
    std::ifstream file(srcFileName , std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        LOG("Failed to open file: " + srcFileName);
        return false;
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

    return true;
}

}  // namespace lonelycube::client
