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

#include "core/pch.h"

namespace lonelycube::client {

struct DescriptorLayoutBuilder
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void addBinding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(
        VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr,
        VkDescriptorSetLayoutCreateFlags flags = 0
    );
};

struct DescriptorAllocator
{
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool pool;

    void initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
    void clearDescriptors(VkDevice device);
    void destroyPool(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};

class DescriptorAllocatorGrowable
{
public:
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    void init(VkDevice device, uint32_t initialMaxSets, std::span<PoolSizeRatio> poolRatios);
    void clearPools(VkDevice device);
    void destroyPools(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);

private:
    std::vector<PoolSizeRatio> m_ratios;
    std::vector<VkDescriptorPool> m_fullPools;
    std::vector<VkDescriptorPool> m_readyPools;
    uint32_t m_setsPerPool;

    VkDescriptorPool getPool(VkDevice device);
    VkDescriptorPool createPool(
        VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios
    );
};

class DescriptorWriter
{
public:
    void writeImage(
        int binding, VkImageView image, VkSampler sampler, VkDescriptorType type,
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED
    );
    void writeBuffer(
        int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type
    );
    void clear();
    void updateSet(VkDevice device, VkDescriptorSet set);

private:
    std::deque<VkDescriptorImageInfo> m_imageInfos;
    std::deque<VkDescriptorBufferInfo> m_bufferInfos;
    std::vector<VkWriteDescriptorSet> m_writes;
};

}  // namespace lonelycube::client
