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

#include "errors.hpp"

namespace intvlk
{
	class SwapchainData
	{
	public:
		SwapchainData(const vk::raii::PhysicalDevice& physicalDevice,
			const vk::raii::Device& device,
			const vk::raii::SurfaceKHR& surface,
			const vk::Extent2D& _extent,
			vk::ImageUsageFlags usage,
			const vk::raii::SwapchainKHR* oldSwapchain,
			uint32_t graphicsQueueFamilyIndex,
			uint32_t presentQueueFamilyIndex,
			vk::PresentModeKHR desiredPresentMode)
		{
			vk::SurfaceFormatKHR surfaceFormat{ pickSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface)) };
			colorFormat = surfaceFormat.format;

			vk::SurfaceCapabilitiesKHR surfaceCapabilities{ physicalDevice.getSurfaceCapabilitiesKHR(surface) };
			vk::Extent2D swapchainExtent{ surfaceCapabilities.currentExtent };
			if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
			{
				swapchainExtent.width = std::clamp(_extent.width,
					surfaceCapabilities.minImageExtent.width,
					surfaceCapabilities.maxImageExtent.width);
				swapchainExtent.height = std::clamp(_extent.height,
					surfaceCapabilities.minImageExtent.height,
					surfaceCapabilities.maxImageExtent.height);
			}
			if (swapchainExtent.width == 0 || swapchainExtent.height == 0)
			{
				throw SwapchainZeroDimensionError{ "Swapchain extent is zero" };
			}
			extent = swapchainExtent;
			vk::SurfaceTransformFlagBitsKHR preTransform{
				(surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
					? vk::SurfaceTransformFlagBitsKHR::eIdentity
					: surfaceCapabilities.currentTransform };
			vk::CompositeAlphaFlagBitsKHR compositeAlpha{
				(surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
					? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
				: (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
					? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
				: (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
					? vk::CompositeAlphaFlagBitsKHR::eInherit
					: vk::CompositeAlphaFlagBitsKHR::eOpaque };
			vk::PresentModeKHR presentMode{ pickPresentMode(physicalDevice.getSurfacePresentModesKHR(surface),
														   desiredPresentMode) };
			vk::SwapchainCreateInfoKHR swapchainCreateInfo{
				vk::SwapchainCreateFlagsKHR{},
				surface,
				std::clamp(3U, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount),
				colorFormat,
				surfaceFormat.colorSpace,
				swapchainExtent,
				1,
				usage,
				vk::SharingMode::eExclusive,
				{},
				preTransform,
				compositeAlpha,
				presentMode,
				vk::True,
				oldSwapchain ? **oldSwapchain : nullptr };
			if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
			{
				std::vector<uint32_t> queueFamilyIndices{ graphicsQueueFamilyIndex, presentQueueFamilyIndex };
				swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
				swapchainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
				swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
			}
			swapchain = vk::raii::SwapchainKHR{ device, swapchainCreateInfo };

			images = swapchain.getImages();

			imageViews.reserve(images.size());
			vk::ImageViewCreateInfo imageViewCreateInfo{
				vk::ImageViewCreateFlags{},
				vk::Image{},
				vk::ImageViewType::e2D,
				colorFormat,
				vk::ComponentMapping{},
				vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1} };
			for (const auto& image : images)
			{
				imageViewCreateInfo.image = image;
				imageViews.emplace_back(device, imageViewCreateInfo);
			}
		}

		vk::Format colorFormat{};
		vk::Extent2D extent{};
		vk::raii::SwapchainKHR swapchain{ VK_NULL_HANDLE };
		std::vector<vk::Image> images{};
		std::vector<vk::raii::ImageView> imageViews{};
	};
}
