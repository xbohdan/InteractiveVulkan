#pragma once

// Copyright(c) 2025, Bohdan Soproniuk
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

namespace apps::hamming_neighbor
{
    enum class PipelineType
    {
        GenerateHashes,
        FindHammingNeighbors
    };

    class Hash
    {
    public:
        uint32_t id{};
        uint32_t hash1{};
        uint32_t hash2{};
    };

    class PushConsts
    {
    public:
        vk::DeviceAddress symbolBufferAddress{};
        vk::DeviceAddress hashBufferAddress{};
        vk::DeviceAddress resultBufferAddress{};
    };

    class PushConstants
    {
    public:
        enum class Algorithm : uint32_t
        {
            eLocalBitonicMergeSort = 0,
            eLocalDisperse = 1,
            eGlobalFlip = 2,
            eGlobalDisperse = 3
        };

        uint32_t h;
        Algorithm algorithm;
        vk::DeviceAddress hashBufferAddress;
    };

    class HammingNeighbor final : public apps::vulkan_app::VulkanApp
    {
    public:
        explicit HammingNeighbor(std::string fileName);

        ~HammingNeighbor() override;

        void run() override;

    private:
        std::tuple<uint32_t, vk::raii::PipelineLayout, vk::raii::Pipeline> makePipeline(PipelineType pipelineType) const;

        std::pair<vk::raii::PipelineLayout, vk::raii::Pipeline> makeSortHashesPipeline(uint32_t pushConstantSize, uint32_t workGroupSize) const;
        std::tuple<uint32_t, uint32_t, std::vector<uint32_t>> readFile(std::string_view fileName) const;
        void runPipeline(PipelineType pipelineType);
        void runSortHashesPipeline();

        void sortHashes();

        std::string fileName;
        uint32_t count{};
        uint32_t length{};

        vk::raii::Context context{};
        vk::raii::Instance instance;
#if !defined(NDEBUG)
        vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger;
#endif
        vk::raii::PhysicalDevice physicalDevice;
        uint32_t computeQueueFamilyIndex;
        vk::raii::Device device;
        vk::raii::CommandPool commandPool;
        vk::raii::Queue computeQueue;
        std::shared_ptr<VmaAllocator_T> allocator;
        intvlk::vma_utils::BufferData symbolBufferData{nullptr};
        vk::DeviceAddress symbolBufferAddress{};
        intvlk::vma_utils::BufferData hashBufferData{nullptr};
        vk::DeviceAddress hashBufferAddress{};
        intvlk::vma_utils::BufferData resultBufferData{nullptr};
        vk::DeviceAddress resultBufferAddress{};
        intvlk::glslang_utils::GlslangContext glslContext{};
    };
}
