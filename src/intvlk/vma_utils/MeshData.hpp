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

#include "BufferData.hpp"

#include <glm/glm.hpp>

namespace intvlk::vma_utils
{
	class MeshData
	{
	public:
		MeshData(const vk::raii::Device& device,
			const VmaAllocator& allocator,
			vk::DeviceSize indexBufferSize,
			vk::DeviceSize vertexBufferSize)
			: indexBuffer{ makeIndexBuffer(device, allocator, indexBufferSize) },

			vertexBuffer{ device,
						 allocator,
						 vertexBufferSize,
						 vk::BufferUsageFlagBits::eVertexBuffer |
							 vk::BufferUsageFlagBits::eTransferDst |
							 vk::BufferUsageFlagBits::eShaderDeviceAddress,
						 VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
						 VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT }
		{
			vk::BufferDeviceAddressInfo bufferDeviceAddressInfo{};
			bufferDeviceAddressInfo.buffer = *vertexBuffer.buffer;
			vertexBufferAddress = device.getBufferAddress(bufferDeviceAddressInfo);
		}

		MeshData(const vk::raii::Device& device,
			const VmaAllocator& allocator,
			vk::DeviceSize vertexBufferSize)
			: MeshData{ device, allocator, 0, vertexBufferSize } {}

		static BufferData makeIndexBuffer(const vk::raii::Device& device,
			const VmaAllocator& allocator,
			vk::DeviceSize indexBufferSize)
		{
			if (indexBufferSize > 0)
			{
				return BufferData{ device,
								  allocator,
								  indexBufferSize,
								  vk::BufferUsageFlagBits::eIndexBuffer |
									  vk::BufferUsageFlagBits::eTransferDst,
								  VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
								  VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT };
			}
			return BufferData{ nullptr };
		}

		BufferData indexBuffer{ nullptr };
		BufferData vertexBuffer{ nullptr };
		vk::DeviceAddress vertexBufferAddress{};
	};
}
