#pragma once

// Copyright(c) 2019, NVIDIA CORPORATION. All rights reserved.
// Copyright(c) 2024-2025, Bohdan Soproniuk
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

namespace intvlk
{
    class PerFrameData
    {
    public:
        PerFrameData(const vk::raii::Device &device,
                     const std::shared_ptr<VmaAllocator_T> &allocator,
                     vk::Format format,
                     vk::Extent2D extent,
                     uint32_t queueFamilyIndex)
            : drawImage{device,
                        allocator,
                        format,
                        extent,
                        vk::ImageTiling::eOptimal,
                        vk::ImageUsageFlagBits::eTransferSrc |
                            vk::ImageUsageFlagBits::eTransferDst |
                            vk::ImageUsageFlagBits::eStorage |
                            vk::ImageUsageFlagBits::eColorAttachment,
                        vk::ImageLayout::eUndefined,
                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                        vk::ImageAspectFlagBits::eColor},

              depthAttachmentData{device, allocator, vk::Format::eD32Sfloat, extent},

              commandPool{device, vk::CommandPoolCreateInfo{vk::CommandPoolCreateFlags{}, queueFamilyIndex}},

              commandBuffer{makeCommandBuffer(device, commandPool)},

              fence{device, vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}},

              acquireSemaphore{device, vk::SemaphoreCreateInfo{}}
        {
        }

        static std::vector<PerFrameData> make(uint32_t queuedFramesCount,
                                              const vk::raii::Device &device,
                                              const std::shared_ptr<VmaAllocator_T> &allocator,
                                              vk::Format format,
                                              vk::Extent2D extent,
                                              uint32_t queueFamilyIndex)
        {
            std::vector<PerFrameData> perFrameData{};
            perFrameData.reserve(queuedFramesCount);
            for (size_t i{0}; i < queuedFramesCount; ++i)
            {
                perFrameData.emplace_back(device, allocator, format, extent, queueFamilyIndex);
            }
            return perFrameData;
        }

        intvlk::vma_utils::ImageData drawImage;
        intvlk::vma_utils::DepthAttachmentData depthAttachmentData;
        vk::raii::CommandPool commandPool{VK_NULL_HANDLE};
        vk::raii::CommandBuffer commandBuffer{nullptr};
        vk::raii::Fence fence{VK_NULL_HANDLE};
        vk::raii::Semaphore acquireSemaphore{VK_NULL_HANDLE};
    };
}
