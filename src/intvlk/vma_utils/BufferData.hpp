#pragma once

// Copyright(c) 2019, NVIDIA CORPORATION. All rights reserved.
// Copyright(c) 2024, Bohdan Soproniuk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Modifications:
// - Modified by Bohdan Soproniuk in 2024 to form a basis for a
// Vulkan-based interactive 3D graphics and computing application.
//

#include "include.hpp"

#include "utils.hpp"

#include "../utils.hpp"

namespace intvlk::vma_utils
{
    class BufferData
    {
    public:
        BufferData(const vk::raii::Device& device,
            const std::shared_ptr<VmaAllocator_T>& _allocator,
            vk::DeviceSize _size,
            vk::BufferUsageFlags _bufferUsage,
            VmaMemoryUsage usage,
            VmaAllocationCreateFlags flags = 0)
            : allocator{ _allocator }

#if !defined(NDEBUG)
            ,
            size{ _size },

            bufferUsage{ _bufferUsage }
#endif
        {
            std::tie(buffer, allocation) = makeBufferAllocation(device, allocator, _size, _bufferUsage, usage, flags);

            vmaGetAllocationInfo(allocator.get(), allocation.get(), &allocationInfo);

            VkMemoryPropertyFlags _propertyFlags{};
            vmaGetAllocationMemoryProperties(allocator.get(), allocation.get(), &_propertyFlags);
            propertyFlags = vk::MemoryPropertyFlags{ _propertyFlags };
        }

        explicit BufferData(std::nullptr_t) {}

        static std::pair<vk::raii::Buffer, std::shared_ptr<VmaAllocation_T>> makeBufferAllocation(
            const vk::raii::Device& device,
            const std::shared_ptr<VmaAllocator_T>& allocator,
            vk::DeviceSize _size,
            vk::BufferUsageFlags _bufferUsage,
            VmaMemoryUsage usage,
            VmaAllocationCreateFlags flags = 0)
        {
            vk::BufferCreateInfo bufferCreateInfo{ vk::BufferCreateFlags{}, _size, _bufferUsage };
            VkBufferCreateInfo _bufferCreateInfo = bufferCreateInfo;
            VkBuffer _buffer{ VK_NULL_HANDLE };

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = usage;
            allocationCreateInfo.flags = flags | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            VmaAllocation _allocation{ nullptr };
            vmaCreateBuffer(allocator.get(),
                &_bufferCreateInfo,
                &allocationCreateInfo,
                &_buffer,
                &_allocation,
                nullptr);

            vk::raii::Buffer buffer{ vk::raii::Buffer{device, _buffer} };
            std::shared_ptr<VmaAllocation_T> allocation{ std::shared_ptr<VmaAllocation_T>{_allocation,
                                                                                         [allocator](VmaAllocation a)
                                                                                         { vmaFreeMemory(allocator.get(), a); }} };
            return { std::move(buffer), allocation };
        }

        template <typename DataType>
        void upload(const vk::raii::Device& device,
            const vk::raii::CommandPool& commandPool,
            const vk::raii::Queue queue,
            const std::vector<DataType>& data,
            size_t stride = 0) const
        {
            if (propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
            {
                size_t elementSize{ stride ? stride : sizeof(DataType) };
                assert(sizeof(DataType) <= elementSize);

                copyToDevice(allocator.get(), allocation.get(), std::span{ data }, elementSize);
            }
            else
            {
                assert(bufferUsage & vk::BufferUsageFlagBits::eTransferDst);
                assert(propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal);

                size_t elementSize{ stride ? stride : sizeof(DataType) };
                assert(sizeof(DataType) <= elementSize);

                size_t dataSize{ data.size() * elementSize };
                assert(dataSize <= size);

                BufferData stagingBuffer{ device,
                                         allocator,
                                         dataSize,
                                         vk::BufferUsageFlagBits::eTransferSrc,
                                         VMA_MEMORY_USAGE_AUTO,
                                         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT };
                copyToDevice(allocator.get(), stagingBuffer.allocation.get(), std::span{ data }, elementSize);
                vmaFlushAllocation(allocator.get(), stagingBuffer.allocation.get(), 0, dataSize);

                oneTimeSubmit(device, commandPool, queue, [&](const vk::raii::CommandBuffer& commandBuffer)
                    { commandBuffer.copyBuffer(stagingBuffer.buffer, buffer, vk::BufferCopy{ 0, 0, dataSize }); });
            }
        }

        const std::shared_ptr<VmaAllocator_T>& allocator{ nullptr };
        std::shared_ptr<VmaAllocation_T> allocation{ nullptr };
        vk::raii::Buffer buffer{ VK_NULL_HANDLE };
        VmaAllocationInfo allocationInfo{};
        vk::MemoryPropertyFlags propertyFlags{};
#if !defined(NDEBUG)
    private:
        vk::DeviceSize size{};
        vk::BufferUsageFlags bufferUsage{};
#endif
    };
}
