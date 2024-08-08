#pragma once

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

#include "include.hpp"

#include "../errors.hpp"

namespace intvlk::vma_utils
{
    class VmaContext
    {
    public:
        VmaContext(VmaAllocatorCreateFlags flags,
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
            if (vmaCreateAllocator(&allocatorInfo, &allocator))
            {
                throw Error{ "Failed to create VMA allocator" };
            }
        }

        VmaContext(const VmaContext&) = delete;
        VmaContext& operator=(const VmaContext&) = delete;

        VmaContext(VmaContext&&) = delete;
        VmaContext& operator=(VmaContext&&) = delete;

        ~VmaContext()
        {
            vmaDestroyAllocator(allocator);
        }

        VmaAllocator allocator{ nullptr };
    };
}
