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

#include "HammingNeighbors.hpp"

namespace apps::hamming_neighbors
{
    HammingNeighbors::HammingNeighbors(std::string fileName)
        : VulkanApp{"Hamming Neighbors"},

          fileName{std::move(fileName)},

          instance{
              intvlk::makeInstance(
                  context,
                  getAppName(),
                  "No Engine",
                  {},
                  {},
                  vk::ApiVersion13,
                  nullptr)},

#if !defined(NDEBUG)
          debugUtilsMessenger{instance, intvlk::makeDebugUtilsMessengerCreateInfo()},
#endif

          physicalDevice{intvlk::findPhysicalDevice(instance)},

          computeQueueFamilyIndex{intvlk::findQueueFamilyIndex(physicalDevice, vk::QueueFlagBits::eCompute)},

          device{intvlk::makeDevice(physicalDevice, {}, computeQueueFamilyIndex)},

          commandPool{device, vk::CommandPoolCreateInfo{vk::CommandPoolCreateFlags{}, computeQueueFamilyIndex}},

          computeQueue{device, computeQueueFamilyIndex, 0},

          allocator{
              intvlk::vma_utils::makeAllocator(
                  VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                  physicalDevice,
                  device,
                  instance,
                  vk::ApiVersion13)}

    {
        assert(!this->fileName.empty());
    }

    HammingNeighbors::~HammingNeighbors()
    {
        device.waitIdle();
    }

    void HammingNeighbors::run()
    {
        std::vector<uint32_t> data{};
        std::tie(count, length, data) = readFile(fileName);

        symbolBufferData = intvlk::vma_utils::BufferData{
            device,
            allocator,
            count * length * sizeof(uint32_t),
            vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eShaderDeviceAddress,
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            {},
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT};

        symbolBufferData.upload(device, commandPool, computeQueue, data);
        symbolBufferAddress = device.getBufferAddress(vk::BufferDeviceAddressInfo{symbolBufferData.buffer});

        hashBufferData = intvlk::vma_utils::BufferData{
            device,
            allocator,
            count * sizeof(Hash),
            vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eShaderDeviceAddress,
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            {},
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT};

        hashBufferAddress = device.getBufferAddress(vk::BufferDeviceAddressInfo{hashBufferData.buffer});

        resultBufferData = intvlk::vma_utils::BufferData{
            device,
            allocator,
            sizeof(uint32_t),
            vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eShaderDeviceAddress,
            VMA_MEMORY_USAGE_AUTO,
            {},
            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT};

        resultBufferAddress = device.getBufferAddress(vk::BufferDeviceAddressInfo{resultBufferData.buffer});

        runPipeline(PipelineType::GenerateHashes);

        intvlk::vma_utils::BufferData hostBufferData{
            device,
            allocator,
            count * sizeof(Hash),
            vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eShaderDeviceAddress,
            VMA_MEMORY_USAGE_AUTO,
            {},
            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT};

        runSortHashesPipeline();

        runPipeline(PipelineType::FindHammingNeighbors);

        const auto *result{static_cast<const uint32_t *>(resultBufferData.allocationInfo.pMappedData)};
        std::cout << "Result: " << *result << "\n";
    }

    std::tuple<uint32_t, vk::raii::PipelineLayout, vk::raii::Pipeline> HammingNeighbors::makePipeline(PipelineType pipelineType) const
    {
        vk::PushConstantRange pushConstantRange{vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushConsts)};

        vk::raii::PipelineLayout computePipelineLayout{
            device,
            vk::PipelineLayoutCreateInfo{vk::PipelineLayoutCreateFlags{}, {}, pushConstantRange}};

        const std::vector<vk::SpecializationMapEntry> specializationMapEntries{
            {0, 0, sizeof(uint32_t)},
            {1, sizeof(uint32_t), sizeof(uint32_t)},
            {2, 2 * sizeof(uint32_t), sizeof(uint32_t)}};

        const uint32_t workGroupSize{physicalDevice.getProperties().limits.maxComputeWorkGroupSize[0]};

        const std::vector<uint32_t> specializationData{workGroupSize, count, length};

        vk::SpecializationInfo specializationInfo{
            static_cast<uint32_t>(specializationMapEntries.size()),
            specializationMapEntries.data(),
            specializationData.size() * sizeof(uint32_t),
            specializationData.data()};

        std::string shaderPath{pipelineType == PipelineType::GenerateHashes
                                   ? "/Users/skylar/xcode/InteractiveVulkan/src/apps/HammingNeighbors/generate_hashes.comp"
                                   : "/Users/skylar/xcode/InteractiveVulkan/src/apps/HammingNeighbors/find_hamming_neighbors.comp"};

        vk::raii::ShaderModule computeShaderModule{
            glslContext.makeShaderModule(
                device,
                vk::ShaderStageFlagBits::eCompute,
                intvlk::readFile(shaderPath))};

        vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{
            vk::PipelineShaderStageCreateFlags{},
            vk::ShaderStageFlagBits::eCompute,
            computeShaderModule,
            "main",
            &specializationInfo};

        vk::ComputePipelineCreateInfo computePipelineCreateInfo{
            vk::PipelineCreateFlags{},
            pipelineShaderStageCreateInfo,
            computePipelineLayout};

        vk::raii::Pipeline computePipeline{device, nullptr, computePipelineCreateInfo};

        return {workGroupSize, std::move(computePipelineLayout), std::move(computePipeline)};
    }

    std::pair<vk::raii::PipelineLayout, vk::raii::Pipeline> HammingNeighbors::makeSortHashesPipeline(uint32_t pushConstantSize, uint32_t workGroupSize) const
    {
        vk::PushConstantRange pushConstantRange{vk::ShaderStageFlagBits::eCompute, 0, pushConstantSize};

        vk::raii::PipelineLayout computePipelineLayout{
            device,
            vk::PipelineLayoutCreateInfo{vk::PipelineLayoutCreateFlags{}, {}, pushConstantRange}};

        const vk::SpecializationMapEntry specializationMapEntry{0, 0, sizeof(uint32_t)};

        vk::SpecializationInfo specializationInfo{1, &specializationMapEntry, sizeof(uint32_t), &workGroupSize};

        // std::vector<uint32_t> shaderCode{intvlk::readBinaryFile("src/shaders/sortHashes.comp.spv")};

        vk::raii::ShaderModule computeShaderModule{
            glslContext.makeShaderModule(
                device,
                vk::ShaderStageFlagBits::eCompute,
                intvlk::readFile("/Users/skylar/xcode/InteractiveVulkan/src/apps/HammingNeighbors/sort_hashes.comp"))};

        vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{
            vk::PipelineShaderStageCreateFlags{},
            vk::ShaderStageFlagBits::eCompute,
            computeShaderModule,
            "main",
            &specializationInfo};

        vk::ComputePipelineCreateInfo computePipelineCreateInfo{
            vk::PipelineCreateFlags{},
            pipelineShaderStageCreateInfo,
            computePipelineLayout};

        vk::raii::Pipeline computePipeline{device, nullptr, computePipelineCreateInfo};

        return {std::move(computePipelineLayout), std::move(computePipeline)};
    }

    std::tuple<uint32_t, uint32_t, std::vector<uint32_t>> HammingNeighbors::readFile(std::string_view fileName) const
    {
        std::ifstream file{std::string{fileName}};
        file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        uint32_t count{};
        uint32_t length{};
        file >> count >> length;
        // This bitonic mergesort works for any power-of-two number of elements, up to 1024
        assert(count > 0 && count <= 1024);
        assert((count & (count - 1)) == 0);
        assert(length > 0);

        std::vector<uint32_t> data{};
        data.resize(count * length);
        for (size_t i{0}; i < count; ++i)
        {
            for (size_t j{0}; j < length; ++j)
            {
                char c{};
                file >> c;
                data[i * length + j] = c == '1' ? 1 : 2;
            }
        }
        return {count, length, std::move(data)};
    }

    void HammingNeighbors::runPipeline(PipelineType pipelineType)
    {
        auto [workGroupSize, pipelineLayout, pipeline] = makePipeline(pipelineType);

        vk::raii::CommandBuffer commandBuffer{intvlk::makeCommandBuffer(device, commandPool)};
        commandBuffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
        commandBuffer.pushConstants<PushConsts>(
            pipelineLayout,
            vk::ShaderStageFlagBits::eCompute,
            0,
            PushConsts{
                symbolBufferAddress,
                hashBufferAddress,
                resultBufferAddress});
        uint32_t workGroupCount{(count + workGroupSize - 1) / workGroupSize};
        commandBuffer.dispatch(workGroupCount, 1, 1);
        commandBuffer.end();

        vk::CommandBufferSubmitInfo commandBufferSubmitInfo{commandBuffer};
        vk::SubmitInfo2 submitInfo{vk::SubmitFlags{}, {}, commandBufferSubmitInfo, {}};
        computeQueue.submit2(submitInfo);
        computeQueue.waitIdle();
    }

    void HammingNeighbors::runSortHashesPipeline()
    {
        const uint32_t workGroupSize{std::min(
            physicalDevice.getProperties().limits.maxComputeWorkGroupSize[0],
            count / 2)};

        auto [pipelineLayout, pipeline] = makeSortHashesPipeline(sizeof(PushConstants), workGroupSize);

        vk::raii::CommandBuffer commandBuffer{intvlk::makeCommandBuffer(device, commandPool)};
        commandBuffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);

        uint32_t workGroupCount{count / (workGroupSize * 2)};

        auto dispatch = [workGroupCount, &pipelineLayout, &commandBuffer, this](uint32_t h, PushConstants::Algorithm algorithm)
        {
            commandBuffer.pushConstants<PushConstants>(
                pipelineLayout,
                vk::ShaderStageFlagBits::eCompute,
                0,
                PushConstants{h, algorithm, hashBufferAddress});

            vk::BufferMemoryBarrier bufferMemoryBarrier{
                vk::AccessFlagBits::eShaderWrite,
                vk::AccessFlagBits::eShaderRead,
                computeQueueFamilyIndex,
                computeQueueFamilyIndex,
                hashBufferData.buffer,
                0,
                vk::WholeSize};

            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags{}, {}, bufferMemoryBarrier, {});

            commandBuffer.dispatch(workGroupCount, 1, 1);
        };

        auto localBitonicMergeSort{[&dispatch](uint32_t h)
                                   {
                                       dispatch(h, PushConstants::Algorithm::eLocalBitonicMergeSort);
                                   }};

        auto localDisperse{[&dispatch](uint32_t h)
                           {
                               dispatch(h, PushConstants::Algorithm::eLocalDisperse);
                           }};

        auto globalFlip{[&dispatch](uint32_t h)
                        {
                            dispatch(h, PushConstants::Algorithm::eGlobalFlip);
                        }};

        auto globalDisperse{[&dispatch](uint32_t h)
                            {
                                dispatch(h, PushConstants::Algorithm::eGlobalDisperse);
                            }};

        uint32_t h{workGroupSize * 2};
        const uint32_t n{count};
        assert(h <= n);
        assert(h % 2 == 0);

        localBitonicMergeSort(h);

        h *= 2;

        for (; h <= n; h *= 2)
        {
            globalFlip(h);

            for (uint32_t hh{h / 2}; hh > 1; hh /= 2)
            {
                if (hh <= workGroupSize * 2)
                {
                    localDisperse(hh);
                    break;
                }
                else
                {
                    globalDisperse(hh);
                }
            }
        }

        commandBuffer.end();

        vk::CommandBufferSubmitInfo commandBufferSubmitInfo{commandBuffer};
        vk::SubmitInfo2 submitInfo{vk::SubmitFlags{}, {}, commandBufferSubmitInfo, {}};
        computeQueue.submit2(submitInfo);
        computeQueue.waitIdle();
    }
}
