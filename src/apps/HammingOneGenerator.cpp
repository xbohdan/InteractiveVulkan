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

#include "HammingOneGenerator.hpp"

HammingOneGenerator::HammingOneGenerator(uint32_t createCount, uint32_t changeCount, uint32_t length)
    : createCount{ createCount },

    changeCount{ changeCount },

    length{ length },

    instance{ intvlk::makeInstance(context,
                                  appName,
                                  "No Engine",
                                  {},
                                  {},
                                  vk::ApiVersion13,
                                  nullptr) },

#if !defined(NDEBUG)
    debugUtilsMessenger{ instance, intvlk::makeDebugUtilsMessengerCreateInfo() },
#endif

    physicalDevice{ intvlk::findPhysicalDevice(instance) },

    maxWorkGroupSizeX{ physicalDevice.getProperties().limits.maxComputeWorkGroupSize[0] },

    createGroupCountX{ (createCount * length + maxWorkGroupSizeX - 1) / maxWorkGroupSizeX },

    changeGroupCountX{ (changeCount + maxWorkGroupSizeX - 1) / maxWorkGroupSizeX },

    computeQueueFamilyIndex{ intvlk::findQueueFamilyIndex(physicalDevice, vk::QueueFlagBits::eCompute) },

    device{ intvlk::makeDevice(physicalDevice, {}, computeQueueFamilyIndex) },

    commandPool{ device, vk::CommandPoolCreateInfo{vk::CommandPoolCreateFlags{}, computeQueueFamilyIndex} },

    commandBuffer{ intvlk::makeCommandBuffer(device, commandPool) },

    computeQueue{ device, computeQueueFamilyIndex, 0 },

    allocator{ intvlk::vma_utils::makeAllocator(VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                                               physicalDevice,
                                               device,
                                               instance,
                                               vk::ApiVersion13) },

    deviceBufferData{ device,
                     allocator,
                     createGroupCountX * maxWorkGroupSizeX * sizeof(uint32_t),
                     vk::BufferUsageFlagBits::eStorageBuffer |
                         vk::BufferUsageFlagBits::eTransferSrc |
                         vk::BufferUsageFlagBits::eShaderDeviceAddress,
                     VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                     {},
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                         VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT },

    deviceBufferAddress{ device.getBufferAddress(vk::BufferDeviceAddressInfo{deviceBufferData.buffer}) },

    hostBufferData{ device,
                   allocator,
                   createGroupCountX * maxWorkGroupSizeX * sizeof(uint32_t),
                   vk::BufferUsageFlagBits::eStorageBuffer |
                       vk::BufferUsageFlagBits::eTransferDst |
                       vk::BufferUsageFlagBits::eShaderDeviceAddress,
                   VMA_MEMORY_USAGE_AUTO,
                   {},
                   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT }
{
    vk::PushConstantRange pushConstantRange{ vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushConstants) };

    computePipelineLayout = vk::raii::PipelineLayout{ device, vk::PipelineLayoutCreateInfo{vk::PipelineLayoutCreateFlags{},
                                                                                          {},
                                                                                          pushConstantRange} };

    const std::vector<vk::SpecializationMapEntry> specializationMapEntries{
        {0, 0, sizeof(uint32_t)},
        {1, sizeof(uint32_t), sizeof(uint32_t)},
        {2, 2 * sizeof(uint32_t), sizeof(uint32_t)},
        {3, 3 * sizeof(uint32_t), sizeof(uint32_t)} };

    const std::vector<uint32_t> specializationData{ maxWorkGroupSizeX, createCount, changeCount, length };

    vk::SpecializationInfo specializationInfo{ static_cast<uint32_t>(specializationMapEntries.size()),
                                              specializationMapEntries.data(),
                                              specializationData.size() * sizeof(uint32_t),
                                              specializationData.data() };

    vk::raii::ShaderModule computeShaderModule{ glslContext.makeShaderModule(device,
                                                                            vk::ShaderStageFlagBits::eCompute,
                                                                            intvlk::readFile("src/shaders/hamming_one_generator.comp")) };

    vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{ vk::PipelineShaderStageCreateFlags{},
                                                                    vk::ShaderStageFlagBits::eCompute,
                                                                    computeShaderModule,
                                                                    "main",
                                                                    &specializationInfo };

    vk::ComputePipelineCreateInfo computePipelineCreateInfo{ vk::PipelineCreateFlags{},
                                                            pipelineShaderStageCreateInfo,
                                                            computePipelineLayout };

    computePipeline = vk::raii::Pipeline{ device, nullptr, computePipelineCreateInfo };
}

HammingOneGenerator::~HammingOneGenerator()
{
    device.waitIdle();
}

uint32_t HammingOneGenerator::makeTimeBasedSeed() const
{
    return static_cast<uint32_t>(
        std::chrono::high_resolution_clock::now()
        .time_since_epoch()
        .count()) &
        ((1 << 23) - 1);
}

void HammingOneGenerator::writeData(std::string_view filename, const uint32_t* data, uint32_t createCount, uint32_t length) const
{
    if (std::ofstream file{ std::string{filename} })
    {
        file << createCount << ' ' << length << '\n';
        for (uint32_t i{ 0 }; i < createCount; ++i)
        {
            for (uint32_t j{ 0 }; j < length; ++j)
            {
                file << data[i * length + j];
            }
            file << '\n';
        }
    }
}

void HammingOneGenerator::run()
{
    commandBuffer.begin(vk::CommandBufferBeginInfo{});
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
    PushConstants pushConstants{ makeTimeBasedSeed(), Algorithm::eCreate, deviceBufferAddress };
    commandBuffer.pushConstants<PushConstants>(computePipelineLayout,
        vk::ShaderStageFlagBits::eCompute,
        0,
        pushConstants);
    commandBuffer.dispatch(createGroupCountX, 1, 1);
    vk::BufferMemoryBarrier2 bufferMemoryBarrier{ vk::PipelineStageFlagBits2::eComputeShader,
                                                 vk::AccessFlagBits2::eShaderWrite,
                                                 vk::PipelineStageFlagBits2::eComputeShader,
                                                 vk::AccessFlagBits2::eShaderRead,
                                                 computeQueueFamilyIndex,
                                                 computeQueueFamilyIndex,
                                                 deviceBufferData.buffer,
                                                 0,
                                                 vk::WholeSize };
    commandBuffer.pipelineBarrier2(vk::DependencyInfoKHR(vk::DependencyFlags{},
        {},
        bufferMemoryBarrier,
        {}));
    pushConstants.algorithm = Algorithm::eChange;
    commandBuffer.pushConstants<PushConstants>(computePipelineLayout,
        vk::ShaderStageFlagBits::eCompute,
        0,
        pushConstants);
    commandBuffer.dispatch(changeGroupCountX, 1, 1);
    commandBuffer.end();
    vk::CommandBufferSubmitInfo commandBufferSubmitInfo{ commandBuffer };
    vk::SubmitInfo2 submitInfo{ vk::SubmitFlags{}, {}, commandBufferSubmitInfo, {} };
    computeQueue.submit2(submitInfo);
    computeQueue.waitIdle();
    intvlk::oneTimeSubmit(device, commandPool, computeQueue, [this](const vk::raii::CommandBuffer& cb)
        { cb.copyBuffer(deviceBufferData.buffer, hostBufferData.buffer, vk::BufferCopy{ 0, 0, createGroupCountX * maxWorkGroupSizeX * sizeof(uint32_t) }); });
    const auto* data{ static_cast<const uint32_t*>(hostBufferData.allocationInfo.pMappedData) };
    writeData("hamming_one.txt", data, createCount, length);
}
