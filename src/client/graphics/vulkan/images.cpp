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

#include "client/graphics/vulkan/images.h"

namespace lonelycube::client {

void transitionImage(
    VkCommandBuffer command, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout,
    VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
    VkPipelineStageFlags2 dststageMask, VkAccessFlags2 dstAccessMask
) {
    VkImageMemoryBarrier2 imageBarrier{};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    imageBarrier.srcStageMask = srcStageMask;
    imageBarrier.srcAccessMask = srcAccessMask;
    imageBarrier.dstStageMask = dststageMask;
    imageBarrier.dstAccessMask = dstAccessMask;

    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    VkImageAspectFlags aspectMask;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    imageBarrier.subresourceRange.aspectMask = aspectMask;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    imageBarrier.image = image;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(command, &dependencyInfo);
}

}  // namespace lonelycube::client
