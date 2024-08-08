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

#include "errors.hpp"

namespace intvlk
{
	class SdlContext
	{
	public:
		SdlContext()
		{
			if (SDL_Init(SDL_INIT_VIDEO))
			{
				throw Error{ "Failed to initialize SDL!" };
			}
		}

		SdlContext(const SdlContext&) = delete;
		SdlContext& operator=(const SdlContext&) = delete;

		SdlContext(SdlContext&&) = default;
		SdlContext& operator=(SdlContext&&) = default;

		~SdlContext()
		{
			SDL_Quit();
		}
	};
}
