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

#include "client/graphics/vulkan/descriptors.h"

#include "core/log.h"
#include <vulkan/vulkan_core.h>

namespace lonelycube::client {

void DescriptorLayoutBuilder::addBinding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.binding = binding;
    newBinding.descriptorCount = 1;
    newBinding.descriptorType = type;

    bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::clear()
{
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(
    VkDevice device, VkShaderStageFlags shaderStages, void* pNext,
    VkDescriptorSetLayoutCreateFlags flags
) {
    for (auto& binding : bindings)
        binding.stageFlags |= shaderStages;

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.pNext = pNext;

    createInfo.pBindings = bindings.data();
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.flags = flags;

    VkDescriptorSetLayout layout;
    if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout) != VK_SUCCESS)
        LOG("Failed to create descriptor set layout");

    return layout;
}

bool DescriptorAllocator::initPool(
    VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios
) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(poolRatios.size());
    for (PoolSizeRatio ratio : poolRatios)
    {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * maxSets)
        });
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = 0;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
    {
        LOG("Failed to create descriptor pool");
        return false;
    }

    return true;
}

bool DescriptorAllocator::clearDescriptors(VkDevice device)
{
    if (vkResetDescriptorPool(device, pool, 0) != VK_SUCCESS)
    {
        LOG("Failed to reset descriptor pool");
        return false;
    }

    return true;
}

void DescriptorAllocator::destroyPool(VkDevice device)
{
    vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
        LOG("Failed to allocated descriptor set");

    return descriptorSet;
}

VkDescriptorPool DescriptorAllocatorGrowable::getPool(VkDevice device)
{
    VkDescriptorPool newPool;
    if (m_readyPools.size() != 0)
    {
        newPool = m_readyPools.back();
        m_readyPools.pop_back();
    }
    else
    {
        newPool = createPool(device, m_setsPerPool, m_ratios);
        m_setsPerPool = m_setsPerPool * 1.5;
        if (m_setsPerPool > 4092)
            m_setsPerPool = 4092;
    }

    return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::createPool(
    VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios
) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios)
    {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * setCount)
        });
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = setCount;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    VkDescriptorPool newPool;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &newPool);

    return newPool;
}

void DescriptorAllocatorGrowable::init(
    VkDevice device, uint32_t initialMaxSets, std::span<PoolSizeRatio> poolRatios
) {
    m_ratios.clear();
    for (const auto r : poolRatios)
        m_ratios.push_back(r);

    VkDescriptorPool newPool = createPool(device, initialMaxSets, poolRatios);
    m_setsPerPool = initialMaxSets * 1.5;
    m_readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::clearPools(VkDevice device)
{
    for (auto pool : m_readyPools)
        vkResetDescriptorPool(device, pool, 0);
    for (auto pool : m_fullPools)
    {
        vkResetDescriptorPool(device, pool, 0);
        m_readyPools.push_back(pool);
    }
    m_fullPools.clear();
}

void DescriptorAllocatorGrowable::destroyPools(VkDevice device)
{
    for (auto pool : m_readyPools)
        vkDestroyDescriptorPool(device, pool, nullptr);
    m_readyPools.clear();
    for (auto pool : m_fullPools)
        vkDestroyDescriptorPool(device, pool, nullptr);
    m_fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(
    VkDevice device, VkDescriptorSetLayout layout, void* pNext
) {
    VkDescriptorPool poolToUse = getPool(device);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.pNext = pNext;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        m_fullPools.push_back(poolToUse);
        poolToUse = getPool(device);
        allocInfo.descriptorPool = poolToUse;

        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
        {
            LOG("Failed to allocate descriptor set");
            return VK_NULL_HANDLE;
        }
    }

    m_readyPools.push_back(poolToUse);

    return descriptorSet;
}

}  // namespace lonelycube::client
