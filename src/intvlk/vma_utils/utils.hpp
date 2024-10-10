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

namespace intvlk::vma_utils
{
    template <typename T>
    inline void copyToDevice(const VmaAllocator& allocator,
        const VmaAllocation& allocation,
        std::span<T> data,
        vk::DeviceSize stride = sizeof(T))
    {
        assert(sizeof(T) <= stride);
        VmaAllocationInfo allocationInfo{};
        vmaGetAllocationInfo(allocator, allocation, &allocationInfo);
        auto* deviceData{ static_cast<std::byte*>(allocationInfo.pMappedData) };
        assert(deviceData);
        if (stride == sizeof(T))
        {
            memcpy(deviceData, data.data(), data.size() * sizeof(T));
        }
        else
        {
            for (size_t i{ 0 }; i < data.size(); ++i)
            {
                memcpy(deviceData, &data[i], sizeof(T));
                deviceData += stride;
            }
        }
    }

    template <typename T>
    inline void copyToDevice(const VmaAllocator& allocator, const VmaAllocation& allocation, const T& data)
    {
        copyToDevice(allocator, allocation, std::span{ &data, 1 });
    }

    inline std::shared_ptr<VmaAllocator_T> makeAllocator(VmaAllocatorCreateFlags flags,
        const vk::raii::PhysicalDevice& physicalDevice,
        const vk::raii::Device& device,
        const vk::raii::Instance& instance,
        uint32_t apiVersion)
    {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.flags = flags;
        allocatorInfo.physicalDevice = *physicalDevice;
        allocatorInfo.device = *device;
        allocatorInfo.instance = *instance;
        allocatorInfo.vulkanApiVersion = apiVersion;
        VmaAllocator _allocator{ nullptr };
        if (vmaCreateAllocator(&allocatorInfo, &_allocator))
        {
            throw Error{ "Failed to create VMA allocator" };
        }
        return std::shared_ptr<VmaAllocator_T>{_allocator, vmaDestroyAllocator};
    }
}
