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
        ImageData(const vk::raii::Device &device,
                  const std::shared_ptr<VmaAllocator_T> &_allocator,
                  vk::Format _format,
                  const vk::Extent2D &_extent,
                  vk::ImageTiling tiling,
                  vk::ImageUsageFlags imageUsage,
                  vk::ImageLayout initialLayout,
                  vk::MemoryPropertyFlags requiredMemoryProperties,
                  VmaAllocationCreateFlags allocationFlags,
                  vk::ImageAspectFlags aspectMask)
            : allocator{_allocator},
              format{_format},
              extent{_extent}
        {
            std::tie(image, allocation) = makeImageAllocation(device,
                                                              allocator,
                                                              format,
                                                              extent,
                                                              tiling,
                                                              imageUsage,
                                                              initialLayout,
                                                              requiredMemoryProperties,
                                                              allocationFlags);

            imageView = vk::raii::ImageView{
                device,
                vk::ImageViewCreateInfo{vk::ImageViewCreateFlags{},
                                        image,
                                        vk::ImageViewType::e2D,
                                        format,
                                        vk::ComponentMapping{},
                                        vk::ImageSubresourceRange{aspectMask, 0, 1, 0, 1}}};
        }

        explicit ImageData(std::nullptr_t) {}

        static std::pair<vk::raii::Image, std::shared_ptr<VmaAllocation_T>> makeImageAllocation(
            const vk::raii::Device &device,
            const std::shared_ptr<VmaAllocator_T> &allocator,
            vk::Format format,
            const vk::Extent2D &extent,
            vk::ImageTiling tiling,
            vk::ImageUsageFlags imageUsage,
            vk::ImageLayout initialLayout,
            vk::MemoryPropertyFlags requiredMemoryProperties,
            VmaAllocationCreateFlags allocationFlags)
        {
            vk::ImageCreateInfo imageCreateInfo{vk::ImageCreateFlags{},
                                                vk::ImageType::e2D,
                                                format,
                                                vk::Extent3D{extent, 1},
                                                1,
                                                1,
                                                vk::SampleCountFlagBits::e1,
                                                tiling,
                                                imageUsage | vk::ImageUsageFlagBits::eSampled,
                                                vk::SharingMode::eExclusive,
                                                {},
                                                initialLayout};
            VkImageCreateInfo _imageCreateInfo = imageCreateInfo;
            VkImage _image{VK_NULL_HANDLE};

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            allocationCreateInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(requiredMemoryProperties);
            allocationCreateInfo.flags = allocationFlags;
            VmaAllocation _allocation{nullptr};
            vmaCreateImage(allocator.get(),
                           &_imageCreateInfo,
                           &allocationCreateInfo,
                           &_image,
                           &_allocation,
                           nullptr);

            vk::raii::Image image{vk::raii::Image{device, _image}};
            std::shared_ptr<VmaAllocation_T> allocation{std::shared_ptr<VmaAllocation_T>{
                _allocation,
                [allocator](VmaAllocation a)
                { vmaFreeMemory(allocator.get(), a); }}};
            return {std::move(image), allocation};
        }

        const std::shared_ptr<VmaAllocator_T> &allocator{nullptr};
        std::shared_ptr<VmaAllocation_T> allocation{nullptr};
        vk::raii::Image image{VK_NULL_HANDLE};
        vk::raii::ImageView imageView{VK_NULL_HANDLE};
        vk::Format format{};
        vk::Extent2D extent{};
    };
}
