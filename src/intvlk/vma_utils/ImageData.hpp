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
    class ImageData
    {
    public:
        ImageData(const vk::raii::Device& device,
            const VmaAllocator& _allocator,
            vk::Format _format,
            const vk::Extent2D& _extent,
            vk::ImageTiling tiling,
            vk::ImageUsageFlags usage,
            vk::ImageLayout initialLayout,
            vk::MemoryPropertyFlags memoryProperties,
            vk::ImageAspectFlags aspectMask)
            : allocator{ _allocator },
            format{ _format },
            extent{ _extent }
        {
            vk::ImageCreateInfo imageCreateInfo{ vk::ImageCreateFlags{},
                                                vk::ImageType::e2D,
                                                format,
                                                vk::Extent3D{extent, 1},
                                                1,
                                                1,
                                                vk::SampleCountFlagBits::e1,
                                                tiling,
                                                usage | vk::ImageUsageFlagBits::eSampled,
                                                vk::SharingMode::eExclusive,
                                                {},
                                                initialLayout };
            VkImageCreateInfo _imageCreateInfo = imageCreateInfo;
            VkImage _image{ VK_NULL_HANDLE };

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            allocationCreateInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(memoryProperties);
            vmaCreateImage(allocator,
                &_imageCreateInfo,
                &allocationCreateInfo,
                &_image,
                &allocation,
                nullptr);
            image = vk::raii::Image{ device, _image };

            imageView = vk::raii::ImageView{
                device,
                vk::ImageViewCreateInfo{vk::ImageViewCreateFlags{},
                                        image,
                                        vk::ImageViewType::e2D,
                                        format,
                                        vk::ComponentMapping{},
                                        vk::ImageSubresourceRange{aspectMask, 0, 1, 0, 1}} };
        }

        explicit ImageData(std::nullptr_t) {}

        ~ImageData()
        {
            if (allocator)
            {
                vmaFreeMemory(allocator, allocation);
            }
        }

        const VmaAllocator& allocator{ nullptr };
        vk::Format format{};
        vk::Extent2D extent{};
        VmaAllocation allocation{ nullptr };
        vk::raii::Image image{ VK_NULL_HANDLE };
        vk::raii::ImageView imageView{ VK_NULL_HANDLE };
    };
}
