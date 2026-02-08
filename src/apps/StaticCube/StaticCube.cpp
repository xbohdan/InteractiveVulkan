// Copyright(c) 2024-2025, Bohdan Soproniuk
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

#include "StaticCube.hpp"

#include "DrawPushConstants.hpp"
#include "geometries.hpp"
#include "math.hpp"
#include "Vertex.hpp"

#include <thread>

namespace apps::static_cube
{
    StaticCube::StaticCube(uint32_t width, uint32_t height)
        : VulkanApp{"Static Cube"},

          width{width},

          height{height},

          windowData{getAppName(), vk::Extent2D{width, height}},

          instance{intvlk::makeInstance(context,
                                        getAppName(),
                                        "No Engine",
                                        {},
                                        intvlk::getInstanceExtensions(),
                                        vk::ApiVersion13,
                                        windowData.handle.get())},

#if !defined(NDEBUG)
          debugUtilsMessenger{instance, intvlk::makeDebugUtilsMessengerCreateInfo()},
#endif

          physicalDevice{intvlk::findPhysicalDevice(instance)},

          surface{intvlk::makeSurface(windowData.handle.get(), instance)},

          graphicsAndPresentQueueFamilyIndices{intvlk::findGraphicsAndPresentQueueFamilyIndices(physicalDevice, surface)},

          device{intvlk::makeDevice(physicalDevice,
                                    intvlk::getDeviceExtensions(),
                                    graphicsAndPresentQueueFamilyIndices.first)},

          allocator{intvlk::vma_utils::makeAllocator(VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                                                     physicalDevice,
                                                     device,
                                                     instance,
                                                     vk::ApiVersion13)},

          perFrameData{intvlk::PerFrameData::make(queuedFramesCount,
                                                  device,
                                                  allocator,
                                                  drawImageFormat,
                                                  drawImageExtent,
                                                  graphicsAndPresentQueueFamilyIndices.first)},

          graphicsQueue{device, graphicsAndPresentQueueFamilyIndices.first, 0},

          presentQueue{device, graphicsAndPresentQueueFamilyIndices.second, 0},

          swapchainData{makeSwapchain(true)},

          renderMatrix{createModelViewProjectionClipMatrix(drawImageExtent)},

          meshData{device, allocator, coloredCubeData.size() * sizeof(Vertex)}
    {
        assert(width > 0);
        assert(height > 0);

        meshData.vertexBuffer.upload(
            device,
            vk::raii::CommandPool{
                device,
                vk::CommandPoolCreateInfo{vk::CommandPoolCreateFlags{}, graphicsAndPresentQueueFamilyIndices.first}},
            graphicsQueue,
            coloredCubeData);

        makeGraphicsPipeline();
    }

    StaticCube::~StaticCube()
    {
        device.waitIdle();
    }

    void StaticCube::run()
    {
        SDL_Event e{};

        while (true)
        {
            auto startTime{std::chrono::high_resolution_clock::now()};

            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                {
                    return;
                }
                else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_MINIMIZED)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
            }

            draw();

            frameIndex = (frameIndex + 1) % queuedFramesCount;

            auto endTime{std::chrono::high_resolution_clock::now()};
            accumulatedTime += endTime - startTime;

            ++frameCount;

            if (1000 < std::chrono::duration_cast<std::chrono::milliseconds>(accumulatedTime).count())
            {
                assert(frameCount > 0);

                SDL_SetWindowTitle(windowData.handle.get(),
                                   std::format("{}\tFPS = {}", windowData.getName(), frameCount).c_str());

                accumulatedTime = std::chrono::high_resolution_clock::duration{};
                frameCount = 0;
            }
        }
    }

    void StaticCube::drawGeometry(const vk::raii::CommandBuffer &commandBuffer) const
    {
        vk::RenderingAttachmentInfo colorAttachment{perFrameData[frameIndex].drawImage.imageView,
                                                    vk::ImageLayout::eColorAttachmentOptimal,
                                                    vk::ResolveModeFlagBits::eNone,
                                                    nullptr,
                                                    vk::ImageLayout::eUndefined,
                                                    vk::AttachmentLoadOp::eClear,
                                                    vk::AttachmentStoreOp::eStore,
                                                    vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}}};

        vk::RenderingAttachmentInfo depthAttachment{perFrameData[frameIndex].depthAttachmentData.imageView,
                                                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                                    vk::ResolveModeFlagBits::eNone,
                                                    nullptr,
                                                    vk::ImageLayout::eUndefined,
                                                    vk::AttachmentLoadOp::eClear,
                                                    vk::AttachmentStoreOp::eStore,
                                                    vk::ClearDepthStencilValue{1.0f, 0}};

        vk::RenderingInfo renderingInfo{vk::RenderingFlags{},
                                        vk::Rect2D{vk::Offset2D{0, 0}, perFrameData[frameIndex].drawImage.extent},
                                        1,
                                        0,
                                        colorAttachment,
                                        &depthAttachment};

        commandBuffer.beginRendering(renderingInfo);

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

        DrawPushConstants pushConstants{renderMatrix,
                                        meshData.vertexBufferAddress};

        commandBuffer.pushConstants(pipelineLayout,
                                    vk::ShaderStageFlagBits::eVertex,
                                    0,
                                    vk::ArrayProxy<const DrawPushConstants>{pushConstants});

        vk::Viewport viewport{0.0f,
                              0.0f,
                              static_cast<float>(perFrameData[frameIndex].drawImage.extent.width),
                              static_cast<float>(perFrameData[frameIndex].drawImage.extent.height),
                              0.0f,
                              1.0f};

        commandBuffer.setViewport(0, viewport);

        vk::Rect2D scissor{vk::Offset2D{0, 0}, perFrameData[frameIndex].drawImage.extent};

        commandBuffer.setScissor(0, scissor);

        commandBuffer.draw(static_cast<uint32_t>(coloredCubeData.size()), 1, 0, 0);

        commandBuffer.endRendering();
    }

    void StaticCube::draw()
    {
        while (vk::Result::eTimeout == device.waitForFences(*perFrameData[frameIndex].fence,
                                                            vk::True,
                                                            std::numeric_limits<uint64_t>::max()))
            ;

        vk::Result result{};
        uint32_t backBufferIndex{};

        try
        {
            std::tie(result, backBufferIndex) = swapchainData.swapchain.acquireNextImage(
                std::numeric_limits<uint64_t>::max(),
                perFrameData[frameIndex].acquireSemaphore);
        }
        catch (const vk::OutOfDateKHRError &)
        {
            remakeSwapchain();
            return;
        }
        if (result == vk::Result::eSuboptimalKHR || swapchainData.extent != windowData.getExtent())
        {
            remakeSwapchain();
            return;
        }
        assert(result == vk::Result::eSuccess);

        device.resetFences(*perFrameData[frameIndex].fence);

        perFrameData[frameIndex].commandPool.reset();

        const auto &commandBuffer{perFrameData[frameIndex].commandBuffer};

        commandBuffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

        intvlk::setImageLayout(commandBuffer,
                               perFrameData[frameIndex].drawImage.image,
                               perFrameData[frameIndex].drawImage.format,
                               vk::ImageLayout::eUndefined,
                               vk::ImageLayout::eColorAttachmentOptimal);

        drawGeometry(commandBuffer);

        intvlk::setImageLayout(commandBuffer,
                               perFrameData[frameIndex].drawImage.image,
                               perFrameData[frameIndex].drawImage.format,
                               vk::ImageLayout::eColorAttachmentOptimal,
                               vk::ImageLayout::eTransferSrcOptimal);

        intvlk::setImageLayout(commandBuffer,
                               swapchainData.images[backBufferIndex],
                               swapchainData.colorFormat,
                               vk::ImageLayout::eUndefined,
                               vk::ImageLayout::eTransferDstOptimal);

        commandBuffer.clearColorImage(swapchainData.images[backBufferIndex],
                                      vk::ImageLayout::eTransferDstOptimal,
                                      vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}},
                                      vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

        intvlk::setImageLayout(commandBuffer,
                               swapchainData.images[backBufferIndex],
                               swapchainData.colorFormat,
                               vk::ImageLayout::eTransferDstOptimal,
                               vk::ImageLayout::eTransferDstOptimal);

        intvlk::blitImage(commandBuffer,
                          perFrameData[frameIndex].drawImage.image,
                          perFrameData[frameIndex].drawImage.extent,
                          swapchainData.images[backBufferIndex],
                          swapchainData.extent);

        intvlk::setImageLayout(commandBuffer,
                               swapchainData.images[backBufferIndex],
                               swapchainData.colorFormat,
                               vk::ImageLayout::eTransferDstOptimal,
                               vk::ImageLayout::ePresentSrcKHR);

        commandBuffer.end();

        vk::CommandBufferSubmitInfo commandBufferSubmitInfo{commandBuffer};

        vk::SemaphoreSubmitInfo waitSemaphoreInfo{perFrameData[frameIndex].acquireSemaphore,
                                                  0,
                                                  vk::PipelineStageFlagBits2::eAllTransfer};

        vk::SemaphoreSubmitInfo signalSemaphoreInfo{swapchainData.submitSemaphores[backBufferIndex],
                                                    0,
                                                    vk::PipelineStageFlagBits2::eAllCommands};

        vk::SubmitInfo2 submitInfo{vk::SubmitFlags{}, waitSemaphoreInfo, commandBufferSubmitInfo, signalSemaphoreInfo};

        graphicsQueue.submit2(submitInfo, perFrameData[frameIndex].fence);

        vk::PresentInfoKHR presentInfo{*swapchainData.submitSemaphores[backBufferIndex],
                                       *swapchainData.swapchain,
                                       backBufferIndex};

        try
        {
            result = presentQueue.presentKHR(presentInfo);
        }
        catch (const vk::OutOfDateKHRError &)
        {
            remakeSwapchain();
            return;
        }
        if (result == vk::Result::eSuboptimalKHR || swapchainData.extent != windowData.getExtent())
        {
            remakeSwapchain();
            return;
        }
        assert(result == vk::Result::eSuccess);
    }

    void StaticCube::makeGraphicsPipeline()
    {
        intvlk::glslang_utils::GlslangContext glslContext{};

        vk::PushConstantRange pushConstantRange{vk::ShaderStageFlagBits::eVertex,
                                                0,
                                                sizeof(DrawPushConstants)};

        pipelineLayout = vk::raii::PipelineLayout{
            device,
            vk::PipelineLayoutCreateInfo{vk::PipelineLayoutCreateFlags{}, nullptr, pushConstantRange}};

        vk::raii::ShaderModule vertexShaderModule{glslContext.makeShaderModule(
            device,
            vk::ShaderStageFlagBits::eVertex,
            intvlk::readFile("src/apps/StaticCube/shaders/static_cube.vert"))};

        vk::raii::ShaderModule fragmentShaderModule{glslContext.makeShaderModule(
            device,
            vk::ShaderStageFlagBits::eFragment,
            intvlk::readFile("src/apps/StaticCube/shaders/static_cube.frag"))};

        vk::raii::PipelineCache pipelineCache{device, vk::PipelineCacheCreateInfo{}};

        pipeline = intvlk::makeGraphicsPipeline(device,
                                                pipelineCache,
                                                vertexShaderModule,
                                                nullptr,
                                                fragmentShaderModule,
                                                nullptr,
                                                0,
                                                {},
                                                vk::FrontFace::eClockwise,
                                                true,
                                                pipelineLayout,
                                                perFrameData[0].drawImage.format,
                                                perFrameData[0].depthAttachmentData.format);
    }

    intvlk::SwapchainData StaticCube::makeSwapchain(bool isNew)
    {
        return intvlk::SwapchainData{physicalDevice,
                                     device,
                                     surface,
                                     windowData.getExtent(),
                                     vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
                                     isNew ? nullptr : &swapchainData.swapchain,
                                     graphicsAndPresentQueueFamilyIndices.first,
                                     graphicsAndPresentQueueFamilyIndices.second,
                                     vk::PresentModeKHR::eMailbox};
    }

    void StaticCube::remakeSwapchain()
    {
        vk::SemaphoreSubmitInfo waitSemaphoreInfo{perFrameData[frameIndex].acquireSemaphore,
                                                  0,
                                                  vk::PipelineStageFlagBits2::eNone};

        vk::SubmitInfo2 submitInfo{vk::SubmitFlags{}, waitSemaphoreInfo};

        graphicsQueue.submit2(submitInfo);

        device.waitIdle();

        try
        {
            swapchainData = makeSwapchain(false);
        }
        catch (const intvlk::Error &)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
