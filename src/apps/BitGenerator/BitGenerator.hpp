#pragma once

// Copyright(c) 2025, Bohdan Soproniuk
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

namespace apps::bit_generator
{
    class BitGenerator final : public apps::vulkan_app::VulkanApp
    {
    public:
        BitGenerator(uint32_t createCount, uint32_t changeCount, uint32_t length);

        ~BitGenerator() override;

        void run() override;

    private:
        uint32_t makeTimeBasedSeed() const;
        void writeData(std::string_view fileName, const uint32_t *data) const;

        uint32_t createCount;
        uint32_t changeCount;
        uint32_t length;

        vk::raii::Context context{};
        vk::raii::Instance instance;
#if !defined(NDEBUG)
        vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger;
#endif
        vk::raii::PhysicalDevice physicalDevice;
        uint32_t maxComputeWorkGroupSizeX;
        uint32_t createGroupCountX;
        uint32_t changeGroupCountX;
        uint32_t computeQueueFamilyIndex;
        vk::raii::Device device;
        vk::raii::CommandPool commandPool;
        vk::raii::CommandBuffer commandBuffer;
        vk::raii::Queue computeQueue;
        std::shared_ptr<VmaAllocator_T> allocator;
        intvlk::vma_utils::BufferData deviceBufferData;
        vk::DeviceAddress deviceBufferAddress;
        intvlk::vma_utils::BufferData hostBufferData;
        vk::raii::PipelineLayout computePipelineLayout{VK_NULL_HANDLE};
        intvlk::glslang_utils::GlslangContext glslContext{};
        vk::raii::Pipeline computePipeline{VK_NULL_HANDLE};
    };
}
