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

#include <fstream>
#include <numeric>
#include <unordered_set>

namespace intvlk
{
    inline const char *const khronosValidationLayerName{"VK_LAYER_KHRONOS_validation"};

    inline VKAPI_ATTR vk::Bool32 VKAPI_CALL debugUtilsMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
        void * /*pUserData*/)
    {
#if !defined(NDEBUG)
        if (static_cast<uint32_t>(pCallbackData->messageIdNumber) == 0x822806fa)
        {
            // Validation Warning: vkCreateInstance(): to enable extension VK_EXT_debug_utils, but this extension is intended to support use by applications when
            // debugging and it is strongly recommended that it be otherwise avoided.
            return vk::False;
        }
        else if (static_cast<uint32_t>(pCallbackData->messageIdNumber) == 0xe8d1a9fe)
        {
            // Validation Performance Warning: Using debug builds of the validation layers *will* adversely affect performance.
            return vk::False;
        }
#endif

        std::cerr << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << ": "
                  << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)) << ":\n";
        std::cerr << std::string("\t") << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
        std::cerr << std::string("\t") << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
        std::cerr << std::string("\t") << "message         = <" << pCallbackData->pMessage << ">\n";
        if (0 < pCallbackData->queueLabelCount)
        {
            std::cerr << std::string("\t") << "Queue Labels:\n";
            for (uint32_t i{0}; i < pCallbackData->queueLabelCount; i++)
            {
                std::cerr << std::string("\t\t") << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
            }
        }
        if (0 < pCallbackData->cmdBufLabelCount)
        {
            std::cerr << std::string("\t") << "CommandBuffer Labels:\n";
            for (uint32_t i{0}; i < pCallbackData->cmdBufLabelCount; i++)
            {
                std::cerr << std::string("\t\t") << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
            }
        }
        if (0 < pCallbackData->objectCount)
        {
            std::cerr << std::string("\t") << "Objects:\n";
            for (uint32_t i{0}; i < pCallbackData->objectCount; i++)
            {
                std::cerr << std::string("\t\t") << "Object " << i << "\n";
                std::cerr << std::string("\t\t\t") << "objectType   = " << vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType))
                          << "\n";
                std::cerr << std::string("\t\t\t") << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
                if (pCallbackData->pObjects[i].pObjectName)
                {
                    std::cerr << std::string("\t\t\t") << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
                }
            }
        }
        return vk::False;
    }

    inline void blitImage(const vk::raii::CommandBuffer &commandBuffer,
                          const vk::Image &sourceImage,
                          const vk::Extent2D &sourceExtent,
                          const vk::Image &destinationImage,
                          const vk::Extent2D &destinationExtent)
    {
        float sourceAspectRatio{static_cast<float>(sourceExtent.width) / static_cast<float>(sourceExtent.height)};
        float destinationAspectRatio{static_cast<float>(destinationExtent.width) / static_cast<float>(destinationExtent.height)};
        uint32_t destinationWidth{destinationExtent.width};
        uint32_t destinationHeight{destinationExtent.height};
        if (sourceAspectRatio > destinationAspectRatio)
        {
            destinationHeight = static_cast<uint32_t>(static_cast<float>(destinationWidth) / sourceAspectRatio);
        }
        else
        {
            destinationWidth = static_cast<uint32_t>(static_cast<float>(destinationHeight) * sourceAspectRatio);
        }
        vk::ImageBlit2 blitRegion{
            vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            std::array<vk::Offset3D, 2>{vk::Offset3D{0, 0, 0},
                                        vk::Offset3D{static_cast<int32_t>(sourceExtent.width),
                                                     static_cast<int32_t>(sourceExtent.height),
                                                     1}},
            vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            std::array<vk::Offset3D, 2>{
                vk::Offset3D{static_cast<int32_t>((destinationExtent.width - destinationWidth) / 2),
                             static_cast<int32_t>((destinationExtent.height - destinationHeight) / 2),
                             0},
                vk::Offset3D{static_cast<int32_t>((destinationExtent.width + destinationWidth) / 2),
                             static_cast<int32_t>((destinationExtent.height + destinationHeight) / 2),
                             1}}};
        vk::BlitImageInfo2 blitInfo{sourceImage,
                                    vk::ImageLayout::eTransferSrcOptimal,
                                    destinationImage,
                                    vk::ImageLayout::eTransferDstOptimal,
                                    blitRegion,
                                    vk::Filter::eLinear};
        commandBuffer.blitImage2(blitInfo);
    }

    inline uint32_t findQueueFamilyIndex(const vk::raii::PhysicalDevice &physicalDevice, vk::QueueFlagBits queueFlags)
    {
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties{physicalDevice.getQueueFamilyProperties()};
        assert(queueFamilyProperties.size() < std::numeric_limits<uint32_t>::max());
        auto queueFamilyProperty{
            std::ranges::find_if(queueFamilyProperties, [queueFlags](const vk::QueueFamilyProperties &qfp)
                                 { return static_cast<bool>(qfp.queueFlags & queueFlags); })};
        assert(queueFamilyProperty != queueFamilyProperties.end());
        return static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), queueFamilyProperty));
    }

    inline std::pair<uint32_t, uint32_t> findGraphicsAndPresentQueueFamilyIndices(
        const vk::raii::PhysicalDevice &physicalDevice,
        const vk::raii::SurfaceKHR &surface)
    {
        uint32_t graphicsQueueFamilyIndex{findQueueFamilyIndex(physicalDevice, vk::QueueFlagBits::eGraphics)};
        if (physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, surface))
        {
            return {graphicsQueueFamilyIndex, graphicsQueueFamilyIndex};
        }

        std::vector<vk::QueueFamilyProperties> queueFamilyProperties{physicalDevice.getQueueFamilyProperties()};
        assert(queueFamilyProperties.size() < std::numeric_limits<uint32_t>::max());

        for (size_t i{0}; i < queueFamilyProperties.size(); ++i)
        {
            if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface))
            {
                return {static_cast<uint32_t>(i), static_cast<uint32_t>(i)};
            }
        }

        for (size_t i{0}; i < queueFamilyProperties.size(); ++i)
        {
            if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface))
            {
                return {graphicsQueueFamilyIndex, static_cast<uint32_t>(i)};
            }
        }

        throw Error{"Failed to find a queue family that supports both graphics and present!"};
    }

    inline vk::raii::PhysicalDevice findPhysicalDevice(const vk::raii::Instance &instance)
    {
        vk::raii::PhysicalDevices physicalDevices{instance};
        std::vector<vk::raii::PhysicalDevice> desiredPhysicalDevices{};
        for (const auto &pd : physicalDevices)
        {
            auto physicalDeviceProperties{pd.getProperties()};
            auto supportedFeatures{pd.getFeatures2<vk::PhysicalDeviceFeatures2,
                                                   vk::PhysicalDeviceVulkan12Features,
                                                   vk::PhysicalDeviceVulkan13Features>()};
            const auto &vulkan12Features{supportedFeatures.get<vk::PhysicalDeviceVulkan12Features>()};
            const auto &vulkan13Features{supportedFeatures.get<vk::PhysicalDeviceVulkan13Features>()};
            if (physicalDeviceProperties.apiVersion >= vk::ApiVersion13 &&
                vulkan12Features.bufferDeviceAddress &&
                vulkan12Features.descriptorIndexing &&
                vulkan13Features.dynamicRendering &&
                vulkan13Features.synchronization2)
            {
                desiredPhysicalDevices.emplace_back(pd);
            }
        }
        if (physicalDevices.empty())
        {
            throw Error{"Failed to find a physical device that supports Vulkan 1.3 and the required features!"};
        }
        auto it{std::ranges::find_if(
            physicalDevices,
            [](const vk::raii::PhysicalDevice &pd)
            { return pd.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu; })};
        if (it == physicalDevices.end())
        {
            it = physicalDevices.begin();
        }
        return *it;
    }

    inline void fullPipelineBarrier(const vk::raii::CommandBuffer &commandBuffer)
    {
        vk::MemoryBarrier2 memoryBarrier{vk::PipelineStageFlagBits2::eAllCommands,
                                         vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                                         vk::PipelineStageFlagBits2::eAllCommands,
                                         vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite};
        commandBuffer.pipelineBarrier2(vk::DependencyInfo{vk::DependencyFlags{}, memoryBarrier});
    }

    inline std::vector<const char *> gatherExtensions(const std::vector<std::string> &extensions
#if !defined(NDEBUG)
                                                      ,
                                                      const std::vector<vk::ExtensionProperties> &extensionProperties
#endif
                                                      ,
                                                      SDL_Window *window = nullptr)
    {
        std::vector<const char *> requiredExtensions{};
        for (const auto &ext : extensions)
        {
            requiredExtensions.emplace_back(ext.c_str());
        }
        if (window)
        {
            uint32_t sdlExtensionCount{0};
            SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr);
            std::vector<const char *> sdlExtensions(sdlExtensionCount);
            SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, sdlExtensions.data());
            for (const auto &ext : sdlExtensions)
            {
                requiredExtensions.emplace_back(ext);
            }
        }
        std::unordered_set<std::string_view> uniqueExtensions;
        std::vector<const char *> enabledExtensions{};
        for (const auto &ext : requiredExtensions)
        {
            if (uniqueExtensions.emplace(ext).second)
            {
                assert(std::ranges::any_of(extensionProperties, [ext](const vk::ExtensionProperties &ep)
                                           { return strcmp(ext, ep.extensionName) == 0; }));
                enabledExtensions.emplace_back(ext);
            }
        }
#if !defined(NDEBUG)
        for (std::vector<const char *> debugExtensionNames{vk::EXTDebugUtilsExtensionName,
                                                           vk::EXTLayerSettingsExtensionName};
             const auto &ext : debugExtensionNames)
        {
            if (!uniqueExtensions.contains(ext) &&
                std::ranges::any_of(extensionProperties, [ext](const vk::ExtensionProperties &ep)
                                    { return strcmp(ext, ep.extensionName) == 0; }))
            {
                enabledExtensions.emplace_back(ext);
            }
        }
#endif
        return enabledExtensions;
    }

    inline std::vector<const char *> gatherLayers(const std::vector<std::string> &layers
#if !defined(NDEBUG)
                                                  ,
                                                  const std::vector<vk::LayerProperties> &layerProperties
#endif
    )
    {
        std::vector<const char *> requiredLayers{};
        for (const auto &layer : layers)
        {
            requiredLayers.emplace_back(layer.c_str());
        }
        std::unordered_set<std::string_view> uniqueLayers{};
        std::vector<const char *> enabledLayers{};
        for (const auto &layer : requiredLayers)
        {
            if (uniqueLayers.emplace(layer).second)
            {
                assert(std::ranges::any_of(layerProperties, [layer](const vk::LayerProperties &lp)
                                           { return strcmp(layer, lp.layerName) == 0; }));
                enabledLayers.emplace_back(layer);
            }
        }
#if !defined(NDEBUG)
        if (!uniqueLayers.contains(khronosValidationLayerName) &&
            std::ranges::any_of(layerProperties, [](const vk::LayerProperties &lp)
                                { return strcmp(khronosValidationLayerName, lp.layerName) == 0; }))
        {
            enabledLayers.emplace_back(khronosValidationLayerName);
        }
#endif
        return enabledLayers;
    }

    inline std::vector<std::string> getDeviceExtensions()
    {
        return {vk::KHRSwapchainExtensionName};
    }

    inline std::vector<std::string> getInstanceExtensions()
    {
        std::vector<std::string> extensions{};
        extensions.emplace_back(vk::KHRSurfaceExtensionName);
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        extensions.emplace_back(vk::KHRAndroidSurfaceExtensionName);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
        extensions.emplace_back(vk::EXTMetalSurfaceExtensionName);
#elif defined(VK_USE_PLATFORM_VI_NN)
        extensions.emplace_back(vk::NNViSurfaceExtensionName);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        extensions.emplace_back(vk::KHRWaylandSurfaceExtensionName);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        extensions.emplace_back(vk::KHRWin32SurfaceExtensionName);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        extensions.emplace_back(vk::KHRXcbSurfaceExtensionName);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
        extensions.emplace_back(vk::KHRXlibSurfaceExtensionName);
#elif defined(VK_USE_PLATFORM_XLIB_XRANDR_EXT)
        extensions.emplace_back(vk::EXTAcquireXlibDisplayExtensionName);
#endif
        return extensions;
    }

    inline vk::raii::CommandBuffer makeCommandBuffer(const vk::raii::Device &device,
                                                     const vk::raii::CommandPool &commandPool)
    {
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo{commandPool, vk::CommandBufferLevel::ePrimary, 1};
        return std::move(vk::raii::CommandBuffers{device, commandBufferAllocateInfo}[0]);
    }

    inline vk::DebugUtilsMessengerCreateInfoEXT makeDebugUtilsMessengerCreateInfo()
    {
        return vk::DebugUtilsMessengerCreateInfoEXT{vk::DebugUtilsMessengerCreateFlagsEXT{},
                                                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                                                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
                                                    debugUtilsMessengerCallback};
    }

    inline vk::raii::DescriptorPool makeDescriptorPool(const vk::raii::Device &device,
                                                       const std::vector<vk::DescriptorPoolSize> &poolSizes)
    {
        assert(!poolSizes.empty());
        uint32_t maxSets{std::accumulate(poolSizes.begin(), poolSizes.end(), 0U,
                                         [](uint32_t sum, const vk::DescriptorPoolSize &dps)
                                         { return sum + dps.descriptorCount; })};
        assert(0 < maxSets);

        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                                              maxSets,
                                                              poolSizes};
        return vk::raii::DescriptorPool{device, descriptorPoolCreateInfo};
    }

    inline vk::raii::DescriptorSetLayout makeDescriptorSetLayout(
        const vk::raii::Device &device,
        const std::vector<std::tuple<vk::DescriptorType, uint32_t, vk::ShaderStageFlags>> &bindingData,
        vk::DescriptorSetLayoutCreateFlags flags = {})
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings(bindingData.size());
        for (size_t i{0}; i < bindingData.size(); ++i)
        {
            bindings[i] = vk::DescriptorSetLayoutBinding{static_cast<uint32_t>(i),
                                                         std::get<0>(bindingData[i]),
                                                         std::get<1>(bindingData[i]),
                                                         std::get<2>(bindingData[i])};
        }
        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{flags, bindings};
        return vk::raii::DescriptorSetLayout{device, descriptorSetLayoutCreateInfo};
    }

    inline vk::raii::Device makeDevice(const vk::raii::PhysicalDevice &physicalDevice,
                                       const std::vector<std::string> &extensions,
                                       uint32_t queueFamilyIndex)
    {
        std::vector<const char *> enabledExtensions{};
        enabledExtensions.reserve(extensions.size());
        for (const auto &ext : extensions)
        {
            enabledExtensions.emplace_back(ext.c_str());
        }

        float queuePriority{0.0f};
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{vk::DeviceQueueCreateFlags{},
                                                        queueFamilyIndex,
                                                        1,
                                                        &queuePriority};
        vk::DeviceCreateInfo deviceCreateInfo{vk::DeviceCreateFlags{},
                                              deviceQueueCreateInfo,
                                              {},
                                              enabledExtensions};
        vk::StructureChain deviceCreateInfoChain{deviceCreateInfo,
                                                 vk::PhysicalDeviceVulkan12Features{}
                                                     .setBufferDeviceAddress(vk::True)
                                                     .setDescriptorIndexing(vk::True),
                                                 vk::PhysicalDeviceVulkan13Features{}
                                                     .setDynamicRendering(vk::True)
                                                     .setSynchronization2(vk::True)};
        auto supportedFeatures{physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2,
                                                           vk::PhysicalDeviceVulkan12Features,
                                                           vk::PhysicalDeviceVulkan13Features>()};
        const auto &vulkan12Features{supportedFeatures.get<vk::PhysicalDeviceVulkan12Features>()};
        const auto &vulkan13Features{supportedFeatures.get<vk::PhysicalDeviceVulkan13Features>()};
        assert(vulkan12Features.bufferDeviceAddress && vulkan12Features.descriptorIndexing &&
               vulkan13Features.dynamicRendering && vulkan13Features.synchronization2);
        return vk::raii::Device{physicalDevice, deviceCreateInfoChain.get<vk::DeviceCreateInfo>()};
    }

    inline vk::raii::Pipeline makeGraphicsPipeline(
        const vk::raii::Device &device,
        const vk::raii::PipelineCache &pipelineCache,
        const vk::raii::ShaderModule &vertexShaderModule,
        const vk::SpecializationInfo *vertexShaderSpecializationInfo,
        const vk::raii::ShaderModule &fragmentShaderModule,
        const vk::SpecializationInfo *fragmentShaderSpecializationInfo,
        uint32_t vertexStride,
        const std::vector<std::pair<vk::Format, uint32_t>> &vertexInputAttributeFormatOffset,
        vk::FrontFace frontFace,
        bool depthTested,
        const vk::raii::PipelineLayout &pipelineLayout,
        vk::Format colorFormat,
        vk::Format depthFormat)
    {
        std::array<vk::PipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos{
            vk::PipelineShaderStageCreateInfo{vk::PipelineShaderStageCreateFlags{},
                                              vk::ShaderStageFlagBits::eVertex,
                                              vertexShaderModule,
                                              "main",
                                              vertexShaderSpecializationInfo},
            vk::PipelineShaderStageCreateInfo{vk::PipelineShaderStageCreateFlags{},
                                              vk::ShaderStageFlagBits::eFragment,
                                              fragmentShaderModule,
                                              "main",
                                              fragmentShaderSpecializationInfo}};

        std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions{};
        vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
        vk::VertexInputBindingDescription vertexInputBindingDescription{0, vertexStride};

        if (0 < vertexStride)
        {
            vertexInputAttributeDescriptions.reserve(vertexInputAttributeFormatOffset.size());
            for (uint32_t i{0}; i < vertexInputAttributeFormatOffset.size(); ++i)
            {
                vertexInputAttributeDescriptions.emplace_back(i,
                                                              0,
                                                              vertexInputAttributeFormatOffset[i].first,
                                                              vertexInputAttributeFormatOffset[i].second);
            }
            pipelineVertexInputStateCreateInfo.setVertexBindingDescriptions(vertexInputBindingDescription);
            pipelineVertexInputStateCreateInfo.setVertexAttributeDescriptions(vertexInputAttributeDescriptions);
        }

        vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{
            vk::PipelineInputAssemblyStateCreateFlags{},
            vk::PrimitiveTopology::eTriangleList};

        vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{vk::PipelineViewportStateCreateFlags{},
                                                                            1,
                                                                            nullptr,
                                                                            1,
                                                                            nullptr};

        vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{
            vk::PipelineRasterizationStateCreateFlags{},
            false,
            false,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            frontFace,
            false,
            0.0f,
            0.0f,
            0.0f,
            1.0f};

        vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{
            vk::PipelineMultisampleStateCreateFlags{},
            vk::SampleCountFlagBits::e1};

        vk::StencilOpState stencilOpState{vk::StencilOp::eKeep,
                                          vk::StencilOp::eKeep,
                                          vk::StencilOp::eKeep,
                                          vk::CompareOp::eAlways};
        vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{
            vk::PipelineDepthStencilStateCreateFlags{},
            depthTested,
            depthTested,
            vk::CompareOp::eLessOrEqual,
            false,
            false,
            stencilOpState,
            stencilOpState};

        vk::ColorComponentFlags colorComponentFlags{vk::ColorComponentFlagBits::eR |
                                                    vk::ColorComponentFlagBits::eG |
                                                    vk::ColorComponentFlagBits::eB |
                                                    vk::ColorComponentFlagBits::eA};
        vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{false,
                                                                                vk::BlendFactor::eZero,
                                                                                vk::BlendFactor::eZero,
                                                                                vk::BlendOp::eAdd,
                                                                                vk::BlendFactor::eZero,
                                                                                vk::BlendFactor::eZero,
                                                                                vk::BlendOp::eAdd,
                                                                                colorComponentFlags};
        vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{
            vk::PipelineColorBlendStateCreateFlags{},
            false,
            vk::LogicOp::eNoOp,
            pipelineColorBlendAttachmentState,
            {1.0f, 1.0f, 1.0f, 1.0f}};

        std::array<vk::DynamicState, 2> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{vk::PipelineDynamicStateCreateFlags{},
                                                                          dynamicStates};
        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{0, colorFormat, depthFormat};
        vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo>
            graphicsPipelineCreateInfoChain{{vk::PipelineCreateFlags{},
                                             pipelineShaderStageCreateInfos,
                                             &pipelineVertexInputStateCreateInfo,
                                             &pipelineInputAssemblyStateCreateInfo,
                                             nullptr,
                                             &pipelineViewportStateCreateInfo,
                                             &pipelineRasterizationStateCreateInfo,
                                             &pipelineMultisampleStateCreateInfo,
                                             &pipelineDepthStencilStateCreateInfo,
                                             &pipelineColorBlendStateCreateInfo,
                                             &pipelineDynamicStateCreateInfo,
                                             pipelineLayout},
                                            pipelineRenderingCreateInfo};

        return vk::raii::Pipeline{device,
                                  pipelineCache,
                                  graphicsPipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>()};
    }

    inline
#if defined(NDEBUG)
        vk::StructureChain<vk::InstanceCreateInfo>
#else
        vk::StructureChain<vk::InstanceCreateInfo, vk::LayerSettingsCreateInfoEXT, vk::DebugUtilsMessengerCreateInfoEXT>
#endif
        makeInstanceCreateInfoChain(const vk::ApplicationInfo &applicationInfo,
                                    const std::vector<const char *> &layers,
                                    const std::vector<const char *> &extensions
#if !defined(NDEBUG)
                                    ,
                                    const std::vector<vk::LayerSettingEXT> &layerSettings
#endif
        )
    {
#if defined(NDEBUG)
        vk::StructureChain<vk::InstanceCreateInfo> instanceCreateInfo{
            {vk::InstanceCreateFlags{}, &applicationInfo, layers, extensions}};
#else
        vk::StructureChain<vk::InstanceCreateInfo, vk::LayerSettingsCreateInfoEXT, vk::DebugUtilsMessengerCreateInfoEXT>
            instanceCreateInfo{
                {vk::InstanceCreateFlags{}, &applicationInfo, layers, extensions},
                {layerSettings},
                makeDebugUtilsMessengerCreateInfo()};
#endif
        return instanceCreateInfo;
    }

    inline vk::raii::Instance makeInstance(const vk::raii::Context &context,
                                           const std::string &appName,
                                           const std::string &engineName,
                                           const std::vector<std::string> &layers = {},
                                           const std::vector<std::string> &extensions = {},
                                           uint32_t apiVersion = vk::ApiVersion10,
                                           SDL_Window *window = nullptr)
    {
        vk::ApplicationInfo applicationInfo{appName.c_str(), 1, engineName.c_str(), 1, apiVersion};
        std::vector<const char *> enabledLayers{
            gatherLayers(layers
#if !defined(NDEBUG)
                         ,
                         context.enumerateInstanceLayerProperties()
#endif
                             )};
        std::vector<const char *> enabledExtensions{
            gatherExtensions(extensions
#if !defined(NDEBUG)
                             ,
                             context.enumerateInstanceExtensionProperties()
#endif
                                 ,
                             window)};
#if !defined(NDEBUG)
        std::vector<const char *> validateGpuBasedValues{"GPU_BASED_GPU_ASSISTED"};
        bool validateSyncValues{true};
        bool validateBestPracticesValues{true};
        std::vector<vk::LayerSettingEXT> layerSettings{
            {khronosValidationLayerName, "validate_gpu_based", vk::LayerSettingTypeEXT::eString, validateGpuBasedValues},
            {khronosValidationLayerName, "validate_sync", vk::LayerSettingTypeEXT::eBool32, 1, &validateSyncValues},
            {khronosValidationLayerName, "validate_best_practices", vk::LayerSettingTypeEXT::eBool32, 1, &validateBestPracticesValues}};
#endif
#if defined(NDEBUG)
        vk::StructureChain<vk::InstanceCreateInfo>
            instanceCreateInfoChain{makeInstanceCreateInfoChain(applicationInfo, enabledLayers, enabledExtensions)};
#else
        vk::StructureChain<vk::InstanceCreateInfo, vk::LayerSettingsCreateInfoEXT, vk::DebugUtilsMessengerCreateInfoEXT>
            instanceCreateInfoChain{makeInstanceCreateInfoChain(applicationInfo, enabledLayers, enabledExtensions, layerSettings)};
#endif
        return vk::raii::Instance{context, instanceCreateInfoChain.get<vk::InstanceCreateInfo>()};
    }

    inline vk::raii::Image makeImage(const vk::raii::Device &device)
    {
        vk::ImageCreateInfo imageCreateInfo{vk::ImageCreateFlags{},
                                            vk::ImageType::e2D,
                                            vk::Format::eB8G8R8A8Unorm,
                                            vk::Extent3D{640, 640, 1},
                                            1,
                                            1,
                                            vk::SampleCountFlagBits::e1,
                                            vk::ImageTiling::eLinear,
                                            vk::ImageUsageFlagBits::eTransferSrc};
        return vk::raii::Image{device, imageCreateInfo};
    }

    inline vk::PipelineRenderingCreateInfo makePipelineRenderingCreateInfo(vk::Format colorFormat,
                                                                           vk::Format depthFormat)
    {
        return vk::PipelineRenderingCreateInfo{0, colorFormat, depthFormat};
    }

    inline vk::raii::SurfaceKHR makeSurface(SDL_Window *window, const vk::raii::Instance &instance)
    {
        VkSurfaceKHR _surface{VK_NULL_HANDLE};
        if (!SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(*instance), &_surface))
        {
            throw Error{"Failed to create a Vulkan surface!"};
        }
        return vk::raii::SurfaceKHR{instance, _surface};
    }

    template <typename Func>
    inline void oneTimeSubmit(const vk::raii::Device &device,
                              const vk::raii::CommandPool &commandPool,
                              const vk::raii::Queue &queue,
                              const Func &func)
    {
        vk::raii::CommandBuffer commandBuffer{
            std::move(vk::raii::CommandBuffers{
                device,
                vk::CommandBufferAllocateInfo{commandPool, vk::CommandBufferLevel::ePrimary, 1}}[0])};
        commandBuffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        func(commandBuffer);
        commandBuffer.end();
        vk::CommandBufferSubmitInfo commandBufferSubmitInfo{commandBuffer};
        vk::SubmitInfo2 submitInfo{vk::SubmitFlags{}, nullptr, commandBufferSubmitInfo};
        queue.submit2(submitInfo);
        queue.waitIdle();
    }

    inline vk::Format pickDepthFormat(const vk::raii::PhysicalDevice &physicalDevice)
    {
        for (const auto &format : std::vector<vk::Format>{vk::Format::eD32Sfloat,
                                                          vk::Format::eD32SfloatS8Uint,
                                                          vk::Format::eD24UnormS8Uint})
        {
            vk::FormatProperties props{physicalDevice.getFormatProperties(format)};
            if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            {
                return format;
            }
        }
        throw Error{"Failed to find a supported depth format!"};
    }

    inline vk::PresentModeKHR pickPresentMode(const std::vector<vk::PresentModeKHR> &presentModes,
                                              vk::PresentModeKHR desiredPresentMode)
    {
        if (std::ranges::find(presentModes, desiredPresentMode) != presentModes.end())
        {
            return desiredPresentMode;
        }
        return vk::PresentModeKHR::eFifo;
    }

    inline vk::SurfaceFormatKHR pickSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &formats)
    {
        assert(!formats.empty());
        vk::SurfaceFormatKHR pickedFormat{formats[0]};
        if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
        {
            pickedFormat.format = vk::Format::eB8G8R8A8Unorm;
            pickedFormat.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        }
        else
        {
            std::array<vk::Format, 4> requestedFormats{vk::Format::eB8G8R8A8Unorm,
                                                       vk::Format::eR8G8B8A8Unorm,
                                                       vk::Format::eB8G8R8Unorm,
                                                       vk::Format::eR8G8B8Unorm};
            vk::ColorSpaceKHR requestedColorSpace{vk::ColorSpaceKHR::eSrgbNonlinear};
            for (const auto &requestedFormat : requestedFormats)
            {
                auto it{std::ranges::find_if(
                    formats, [requestedFormat, requestedColorSpace](const vk::SurfaceFormatKHR &f)
                    { return f.format == requestedFormat && f.colorSpace == requestedColorSpace; })};
                if (it != formats.end())
                {
                    pickedFormat = *it;
                    break;
                }
            }
        }
        assert(pickedFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear);
        return pickedFormat;
    }

    inline std::string readFile(std::string_view filename)
    {
        std::string shaderCode{};
        if (std::ifstream file{std::string{filename}, std::ios::ate})
        {
            const auto fileSize{static_cast<size_t>(file.tellg())};
            shaderCode.resize(fileSize);
            file.seekg(0);
            file.read(shaderCode.data(), fileSize);
            return shaderCode;
        }
        throw std::runtime_error("Failed to open file: " + std::string{filename});
    }

    inline void setImageLayout(const vk::raii::CommandBuffer &commandBuffer,
                               vk::Image image,
                               vk::Format format,
                               vk::ImageLayout oldImageLayout,
                               vk::ImageLayout newImageLayout)
    {
        vk::AccessFlags2 sourceAccessMask{};
        switch (oldImageLayout)
        {
        case vk::ImageLayout::eTransferDstOptimal:
            sourceAccessMask = vk::AccessFlagBits2::eTransferWrite;
            break;
        case vk::ImageLayout::ePreinitialized:
            sourceAccessMask = vk::AccessFlagBits2::eHostWrite;
            break;
        case vk::ImageLayout::eGeneral:
        case vk::ImageLayout::eUndefined:
            break;
        default:
            // std::cout << "Unhandled image layout transition!\n";
            sourceAccessMask = vk::AccessFlagBits2::eMemoryWrite;
        }

        vk::PipelineStageFlags2 sourceStage{};
        switch (oldImageLayout)
        {
        case vk::ImageLayout::eGeneral:
        case vk::ImageLayout::ePreinitialized:
            sourceStage = vk::PipelineStageFlagBits2::eHost;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            sourceStage = vk::PipelineStageFlagBits2::eTransfer;
            break;
        case vk::ImageLayout::eUndefined:
            sourceStage = vk::PipelineStageFlagBits2::eTopOfPipe;
            break;
        default:
            // std::cout << "Unhandled image layout transition!\n";
            sourceStage = vk::PipelineStageFlagBits2::eAllCommands;
        }

        vk::AccessFlags2 destinationAccessMask{};
        switch (newImageLayout)
        {
        case vk::ImageLayout::eColorAttachmentOptimal:
            destinationAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            destinationAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                                    vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            destinationAccessMask = vk::AccessFlagBits2::eShaderRead;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            destinationAccessMask = vk::AccessFlagBits2::eTransferRead;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            destinationAccessMask = vk::AccessFlagBits2::eTransferWrite;
            break;
        case vk::ImageLayout::eGeneral:
        case vk::ImageLayout::ePresentSrcKHR:
            break;
        default:
            // std::cout << "Unhandled image layout transition!\n";
            destinationAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
        }

        vk::PipelineStageFlags2 destinationStage{};
        switch (newImageLayout)
        {
        case vk::ImageLayout::eColorAttachmentOptimal:
            destinationStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            destinationStage = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
            break;
        case vk::ImageLayout::eGeneral:
            destinationStage = vk::PipelineStageFlagBits2::eHost;
            break;
        case vk::ImageLayout::ePresentSrcKHR:
            destinationStage = vk::PipelineStageFlagBits2::eBottomOfPipe;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            destinationStage = vk::PipelineStageFlagBits2::eFragmentShader;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
        case vk::ImageLayout::eTransferSrcOptimal:
            destinationStage = vk::PipelineStageFlagBits2::eTransfer;
            break;
        default:
            // std::cout << "Unhandled image layout transition!\n";
            destinationStage = vk::PipelineStageFlagBits2::eAllCommands;
        }

        vk::ImageAspectFlags aspectMask{};
        if (newImageLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            aspectMask = vk::ImageAspectFlagBits::eDepth;
            if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
            {
                aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        }
        else
        {
            aspectMask = vk::ImageAspectFlagBits::eColor;
        }

        vk::ImageSubresourceRange imageSubresourceRange{aspectMask, 0, 1, 0, 1};
        vk::ImageMemoryBarrier2 imageMemoryBarrier{sourceStage,
                                                   sourceAccessMask,
                                                   destinationStage,
                                                   destinationAccessMask,
                                                   oldImageLayout,
                                                   newImageLayout,
                                                   vk::QueueFamilyIgnored,
                                                   vk::QueueFamilyIgnored,
                                                   image,
                                                   imageSubresourceRange};
        commandBuffer.pipelineBarrier2(vk::DependencyInfo{vk::DependencyFlags{},
                                                          nullptr,
                                                          nullptr,
                                                          imageMemoryBarrier});
    }

    inline void submitAndWait(const vk::raii::Device &device,
                              const vk::raii::Queue &queue,
                              const vk::raii::CommandBuffer &commandBuffer)
    {
        vk::raii::Fence fence{device, vk::FenceCreateInfo{}};
        vk::CommandBufferSubmitInfo commandBufferSubmitInfo{commandBuffer};
        queue.submit2(vk::SubmitInfo2{vk::SubmitFlags{}, nullptr, commandBufferSubmitInfo}, fence);
        while (vk::Result::eTimeout == device.waitForFences(*fence, vk::True, std::numeric_limits<uint64_t>::max()))
            ;
    }
}
