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

#include "client/graphics/vulkan/descriptors.h"

#include "client/graphics/vulkan/utils.h"
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
    VK_CHECK(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout));

    return layout;
}

void DescriptorAllocator::initPool(
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

    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool));
}

void DescriptorAllocator::clearDescriptors(VkDevice device)
{
    VK_CHECK(vkResetDescriptorPool(device, pool, 0));
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
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

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

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
    }

    m_readyPools.push_back(poolToUse);

    return descriptorSet;
}

void DescriptorWriter::writeBuffer(
    int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type
) {
    VkDescriptorBufferInfo& info = m_bufferInfos.emplace_back(VkDescriptorBufferInfo{
        .buffer = buffer,
        .offset = offset,
        .range = size
    });

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    m_writes.push_back(write);
}

void DescriptorWriter::writeImage(
    int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type
) {
    VkDescriptorImageInfo& info = m_imageInfos.emplace_back(VkDescriptorImageInfo{
        .sampler = sampler,
        .imageView = image,
        .imageLayout = layout
    });

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    m_writes.push_back(write);
}

void DescriptorWriter::clear()
{
    m_imageInfos.clear();
    m_bufferInfos.clear();
    m_writes.clear();
}

void DescriptorWriter::updateSet(VkDevice device , VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : m_writes)
        write.dstSet = set;

    vkUpdateDescriptorSets(
        device, static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr
    );
}

}  // namespace lonelycube::client
