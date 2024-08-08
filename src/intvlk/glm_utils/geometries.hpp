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

#include "Vertex.hpp"

#include <vector>

namespace intvlk::glm_utils
{
    static const std::vector<Vertex> coloredCubeData{
        // Red face
        {{-1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        // Green face
        {{-1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        // Blue face
        {{-1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-1.0f, -1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-1.0f, -1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        // Yellow face
        {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, -1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        // Magenta face
        {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        // Cyan face
        {{1.0f, -1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        {{1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        {{-1.0f, -1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        {{-1.0f, -1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        {{1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        {{-1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}} };
}
