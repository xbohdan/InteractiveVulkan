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

namespace intvlk::glm_utils
{
	inline glm::mat4x4 createModelViewProjectionClipMatrix(vk::Extent2D const& extent)
	{
		float fov{ glm::radians(45.0f) };
		if (extent.width > extent.height)
		{
			fov *= static_cast<float>(extent.height) / static_cast<float>(extent.width);
		}

		glm::mat4x4 model{ 1.0f };
		glm::mat4x4 view{ glm::lookAt(glm::vec3{-5.0f, 3.0f, -10.0f},
									 glm::vec3{0.0f, 0.0f, 0.0f},
									 glm::vec3{0.0f, -1.0f, 0.0f}) };
		glm::mat4x4 projection{ glm::perspective(fov, 1.0f, 0.1f, 100.0f) };
		glm::mat4x4 clip{ 1.0f, 0.0f, 0.0f, 0.0f,
						 0.0f, -1.0f, 0.0f, 0.0f,
						 0.0f, 0.0f, 0.5f, 0.0f,
						 0.0f, 0.0f, 0.5f, 1.0f }; // Vulkan clip space has inverted y and half z!
		return clip * projection * view * model;
	}
}
