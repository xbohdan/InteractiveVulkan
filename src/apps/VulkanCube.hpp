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

#include "VulkanApp.hpp"

#include <chrono>

class VulkanCube final : public VulkanApp
{
public:
    VulkanCube(const std::string& appName, uint32_t width, uint32_t height);

    ~VulkanCube() override;

    void run() override;

private:
    void drawGeometry(const vk::raii::CommandBuffer& commandBuffer) const;

    void draw();
    void makeGraphicsPipeline();
    intvlk::SwapchainData makeSwapchain(bool isNew);
    void remakeSwapchain();

    const vk::Format drawImageFormat{ vk::Format::eR16G16B16A16Sfloat };
    const vk::Extent2D drawImageExtent{ 1080, 1080 };
    const uint32_t queuedFramesCount{ 2 };

    uint32_t frameIndex{};
    size_t frameCount{};
    std::chrono::high_resolution_clock::duration accumulatedTime{};

    vk::raii::Context context{};
    intvlk::WindowData windowData;
    vk::raii::Instance instance;
#if !defined(NDEBUG)
    vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger;
#endif
    vk::raii::PhysicalDevice physicalDevice;
    vk::raii::SurfaceKHR surface;
    std::pair<uint32_t, uint32_t> graphicsAndPresentQueueFamilyIndices;
    vk::raii::Device device;
    std::shared_ptr<VmaAllocator_T> allocator;
    std::vector<intvlk::PerFrameData> perFrameData;
    vk::raii::Queue graphicsQueue;
    vk::raii::Queue presentQueue;
    intvlk::SwapchainData swapchainData;
    intvlk::vma_utils::ImageData drawImage;
    glm::mat4 renderMatrix;
    intvlk::vma_utils::DepthAttachmentData depthAttachmentData;
    intvlk::vma_utils::MeshData meshData;
    vk::raii::PipelineLayout pipelineLayout{ VK_NULL_HANDLE };
    vk::raii::Pipeline pipeline{ VK_NULL_HANDLE };
};
