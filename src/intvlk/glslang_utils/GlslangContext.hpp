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

#include "../errors.hpp"

#include <iostream>

namespace intvlk::glslang_utils
{
    class GlslangContext
    {
    public:
        GlslangContext()
        {
            glslang::InitializeProcess();
        };

        GlslangContext(const GlslangContext &) = delete;
        GlslangContext &operator=(const GlslangContext &) = delete;

        GlslangContext(GlslangContext &&) = delete;
        GlslangContext &operator=(GlslangContext &&) = delete;

        ~GlslangContext()
        {
            glslang::FinalizeProcess();
        }

        EShLanguage translateShaderStage(vk::ShaderStageFlagBits stage) const
        {
            switch (stage)
            {
            case vk::ShaderStageFlagBits::eVertex:
                return EShLangVertex;
            case vk::ShaderStageFlagBits::eTessellationControl:
                return EShLangTessControl;
            case vk::ShaderStageFlagBits::eTessellationEvaluation:
                return EShLangTessEvaluation;
            case vk::ShaderStageFlagBits::eGeometry:
                return EShLangGeometry;
            case vk::ShaderStageFlagBits::eFragment:
                return EShLangFragment;
            case vk::ShaderStageFlagBits::eCompute:
                return EShLangCompute;
            case vk::ShaderStageFlagBits::eRaygenNV:
                return EShLangRayGenNV;
            case vk::ShaderStageFlagBits::eAnyHitNV:
                return EShLangAnyHitNV;
            case vk::ShaderStageFlagBits::eClosestHitNV:
                return EShLangClosestHitNV;
            case vk::ShaderStageFlagBits::eMissNV:
                return EShLangMissNV;
            case vk::ShaderStageFlagBits::eIntersectionNV:
                return EShLangIntersectNV;
            case vk::ShaderStageFlagBits::eCallableNV:
                return EShLangCallableNV;
            case vk::ShaderStageFlagBits::eTaskNV:
                return EShLangTaskNV;
            case vk::ShaderStageFlagBits::eMeshNV:
                return EShLangMeshNV;
            default:
                assert(false && "Unknown shader stage!");
                return EShLangVertex;
            }
        }

        bool GLSLtoSPV(const vk::ShaderStageFlagBits shaderType,
                       const std::string &glslShader,
                       std::vector<uint32_t> &spvShader) const
        {
            EShLanguage stage{translateShaderStage(shaderType)};

            std::array<const char *, 1> shaderStrings{glslShader.c_str()};

            glslang::TShader shader{stage};
            shader.setStrings(shaderStrings.data(), static_cast<int>(shaderStrings.size()));

            EShMessages messages{static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules)};

            if (!shader.parse(GetDefaultResources(), 100, false, messages))
            {
                std::cerr << shader.getInfoLog() << '\n'
                          << shader.getInfoDebugLog() << '\n';
                return false;
            }

            glslang::TProgram program;
            program.addShader(&shader);

            if (!program.link(messages))
            {
                std::cerr << shader.getInfoLog() << '\n'
                          << shader.getInfoDebugLog() << '\n';
                return false;
            }

            glslang::GlslangToSpv(*program.getIntermediate(stage), spvShader);
            return true;
        }

        template <typename DISPATCHER = VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>
        vk::raii::ShaderModule makeShaderModule(const vk::raii::Device &device,
                                                vk::ShaderStageFlagBits shaderStage,
                                                const std::string &shaderText) const
        {
            std::vector<uint32_t> shaderSPV{};
            if (!GLSLtoSPV(shaderStage, shaderText, shaderSPV))
            {
                throw Error{"Failed to compile shader!"};
            }

            return vk::raii::ShaderModule{device, vk::ShaderModuleCreateInfo{vk::ShaderModuleCreateFlags{}, shaderSPV}};
        }
    };
}
