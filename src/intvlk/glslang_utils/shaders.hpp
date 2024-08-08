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

#include <string>

namespace intvlk::glslang_utils
{
    const std::string vertexShaderText{ R"(
        #version 450

        #extension GL_EXT_buffer_reference : require

        struct Vertex
        {
          vec4 position;
          vec4 color;
        };

        layout (buffer_reference, std430) readonly buffer VertexBuffer
        {
          Vertex vertices[];
        };

        layout (push_constant) uniform PushConstants
        {
          mat4 renderMatrix;
          VertexBuffer vertexBuffer;
        } pushConstants;

        layout (location = 0) out vec4 outColor;

        void main()
        {
          Vertex vertex = pushConstants.vertexBuffer.vertices[gl_VertexIndex];

          outColor = vertex.color;
          gl_Position = pushConstants.renderMatrix * vertex.position;
        }
    )" };

    const std::string fragmentShaderText{ R"(
        #version 450

        layout(location = 0) in vec4 color;

        layout(location = 0) out vec4 outColor;

        void main()
        {
            outColor = color;
        }
    )" };
}
