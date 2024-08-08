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

#include "VulkanApp.hpp"

#include <thread>

VulkanApp::VulkanApp(std::string_view appName, uint32_t width, uint32_t height)
	: windowData{ appName, vk::Extent2D{width, height} },

	instance{ intvlk::makeInstance(context,
								  "Vulkan App",
								  "No Engine",
								  {},
								  intvlk::getInstanceExtensions(),
								  vk::ApiVersion13,
								  windowData.getHandle()) },

#if !defined(NDEBUG)
	debugMessenger{ instance, intvlk::makeDebugUtilsMessengerCreateInfo() },
#endif

	physicalDevice{ intvlk::findPhysicalDevice(instance) },

	surface{ intvlk::makeSurface(windowData.getHandle(), instance)},

	graphicsAndPresentQueueFamilyIndices{ intvlk::findGraphicsAndPresentQueueFamilyIndices(physicalDevice, surface) },

	device{ intvlk::makeDevice(physicalDevice, graphicsAndPresentQueueFamilyIndices.first) },

	vmaCtx{ VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		   physicalDevice,
		   device,
		   instance,
		   vk::ApiVersion13 },

	perFrameData{ intvlk::PerFrameData::make(queuedFramesCount, device, graphicsAndPresentQueueFamilyIndices.first) },

	graphicsQueue{ device, graphicsAndPresentQueueFamilyIndices.first, 0 },

	presentQueue{ device, graphicsAndPresentQueueFamilyIndices.second, 0 },

	swapchainData{ makeSwapchain(true) },

	drawImage{ device,
			  vmaCtx.allocator,
			  drawImageFormat,
			  drawImageExtent,
			  vk::ImageTiling::eOptimal,
			  vk::ImageUsageFlagBits::eTransferSrc |
				  vk::ImageUsageFlagBits::eTransferDst |
				  vk::ImageUsageFlagBits::eStorage |
				  vk::ImageUsageFlagBits::eColorAttachment,
			  vk::ImageLayout::eUndefined,
			  vk::MemoryPropertyFlagBits::eDeviceLocal,
			  vk::ImageAspectFlagBits::eColor },

	renderMatrix{ intvlk::glm_utils::createModelViewProjectionClipMatrix(drawImageExtent) },

	depthAttachmentData{ device, vmaCtx.allocator, vk::Format::eD32Sfloat, drawImage.extent },

	meshData{ device, vmaCtx.allocator, intvlk::glm_utils::coloredCubeData.size() * sizeof(intvlk::glm_utils::Vertex) }
{
	meshData.vertexBuffer.upload(device,
		vk::raii::CommandPool{ device,
							  vk::CommandPoolCreateInfo{vk::CommandPoolCreateFlags{},
														graphicsAndPresentQueueFamilyIndices.first} },
		graphicsQueue,
		intvlk::glm_utils::coloredCubeData);

	makeGraphicsPipeline();
}

VulkanApp::~VulkanApp()
{
	device.waitIdle();
}

void VulkanApp::run()
{
	SDL_Event e{};

	while (true)
	{
		auto startTime{ std::chrono::high_resolution_clock::now() };

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

		auto endTime{ std::chrono::high_resolution_clock::now() };
		accumulatedTime += endTime - startTime;
		++frameCount;
		if (1000 < std::chrono::duration_cast<std::chrono::milliseconds>(accumulatedTime).count())
		{
			assert(0 < frameCount);

			SDL_SetWindowTitle(windowData.getHandle(), std::format("{}\tFPS = {}", windowData.getName(), frameCount).c_str());

			accumulatedTime = std::chrono::high_resolution_clock::duration{};
			frameCount = 0;
		}
	}
}

void VulkanApp::drawGeometry(const vk::raii::CommandBuffer& commandBuffer) const
{
	vk::RenderingAttachmentInfo colorAttachment{ drawImage.imageView, vk::ImageLayout::eColorAttachmentOptimal };

	vk::RenderingAttachmentInfo depthAttachment{ depthAttachmentData.imageView,
												vk::ImageLayout::eDepthStencilAttachmentOptimal,
												vk::ResolveModeFlagBits::eNone,
												nullptr,
												vk::ImageLayout::eUndefined,
												vk::AttachmentLoadOp::eClear,
												vk::AttachmentStoreOp::eStore,
												vk::ClearDepthStencilValue{1.0f, 0} };

	vk::RenderingInfo renderingInfo{ vk::RenderingFlags{},
									vk::Rect2D{vk::Offset2D{0, 0}, drawImage.extent},
									1,
									0,
									colorAttachment,
									&depthAttachment };

	commandBuffer.beginRendering(renderingInfo);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	intvlk::glm_utils::DrawPushConstants pushConstants{ renderMatrix,
													   meshData.vertexBufferAddress };

	commandBuffer.pushConstants(pipelineLayout,
		vk::ShaderStageFlagBits::eVertex,
		0,
		vk::ArrayProxy<const intvlk::glm_utils::DrawPushConstants>{pushConstants});

	vk::Viewport viewport{ 0.0f,
						  0.0f,
						  static_cast<float>(drawImage.extent.width),
						  static_cast<float>(drawImage.extent.height),
						  0.0f,
						  1.0f };

	commandBuffer.setViewport(0, viewport);

	vk::Rect2D scissor{ vk::Offset2D{0, 0}, drawImage.extent };

	commandBuffer.setScissor(0, scissor);

	commandBuffer.draw(static_cast<uint32_t>(intvlk::glm_utils::coloredCubeData.size()), 1, 0, 0);

	commandBuffer.endRendering();
}

void VulkanApp::draw()
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
			perFrameData[frameIndex].presentCompleteSemaphore);
	}
	catch (const vk::OutOfDateKHRError&)
	{
		remakeSwapchain();
		return;
	}
	assert(result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR);

	device.resetFences(*perFrameData[frameIndex].fence);

	perFrameData[frameIndex].commandPool.reset();

	const auto& commandBuffer{ perFrameData[frameIndex].commandBuffer };

	commandBuffer.begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	intvlk::setImageLayout(commandBuffer,
		drawImage.image,
		drawImage.format,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eColorAttachmentOptimal);

	drawGeometry(commandBuffer);

	intvlk::setImageLayout(commandBuffer,
		drawImage.image,
		drawImage.format,
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageLayout::eTransferSrcOptimal);

	intvlk::setImageLayout(commandBuffer,
		swapchainData.images[backBufferIndex],
		swapchainData.colorFormat,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal);

	commandBuffer.clearColorImage(swapchainData.images[backBufferIndex],
		vk::ImageLayout::eTransferDstOptimal,
		vk::ClearColorValue{ std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f} },
		vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

	intvlk::blitImage(commandBuffer,
		drawImage.image,
		drawImage.extent,
		swapchainData.images[backBufferIndex],
		swapchainData.extent);

	intvlk::setImageLayout(commandBuffer,
		swapchainData.images[backBufferIndex],
		swapchainData.colorFormat,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::ePresentSrcKHR);

	commandBuffer.end();

	vk::CommandBufferSubmitInfo commandBufferSubmitInfo{ commandBuffer };

	vk::SemaphoreSubmitInfo waitSemaphoreInfo{ perFrameData[frameIndex].presentCompleteSemaphore,
											  1,
											  vk::PipelineStageFlagBits2::eColorAttachmentOutput };

	vk::SemaphoreSubmitInfo signalSemaphoreInfo{ perFrameData[frameIndex].renderCompleteSemaphore,
												1,
												vk::PipelineStageFlagBits2::eAllGraphics };

	vk::SubmitInfo2 submitInfo{ vk::SubmitFlags{}, waitSemaphoreInfo, commandBufferSubmitInfo, signalSemaphoreInfo };

	graphicsQueue.submit2(submitInfo, perFrameData[frameIndex].fence);

	vk::PresentInfoKHR presentInfo{ *perFrameData[frameIndex].renderCompleteSemaphore,
								   *swapchainData.swapchain,
								   backBufferIndex };

	try
	{
		result = presentQueue.presentKHR(presentInfo);
	}
	catch (const vk::OutOfDateKHRError&)
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

void VulkanApp::makeGraphicsPipeline()
{
	intvlk::glslang_utils::GlslangContext glslCtx{};

	vk::PushConstantRange pushConstantRange{ vk::ShaderStageFlagBits::eVertex,
											0,
											sizeof(intvlk::glm_utils::DrawPushConstants) };
	pipelineLayout = vk::raii::PipelineLayout{
		device,
		vk::PipelineLayoutCreateInfo{vk::PipelineLayoutCreateFlags{}, nullptr, pushConstantRange} };

	vk::raii::ShaderModule vertexShaderModule{ glslCtx.makeShaderModule(device,
																	   vk::ShaderStageFlagBits::eVertex,
																	   intvlk::glslang_utils::vertexShaderText) };
	vk::raii::ShaderModule fragmentShaderModule{ glslCtx.makeShaderModule(device,
																		 vk::ShaderStageFlagBits::eFragment,
																		 intvlk::glslang_utils::fragmentShaderText) };

	vk::raii::PipelineCache pipelineCache{ device, vk::PipelineCacheCreateInfo{} };
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
		drawImage.format,
		depthAttachmentData.format);
}

intvlk::SwapchainData VulkanApp::makeSwapchain(bool isNew)
{
	device.waitIdle();
	return intvlk::SwapchainData{ physicalDevice,
								 device,
								 surface,
								 windowData.getExtent(),
								 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
								 isNew ? nullptr : &swapchainData.swapchain,
								 graphicsAndPresentQueueFamilyIndices.first,
								 graphicsAndPresentQueueFamilyIndices.second,
								 vk::PresentModeKHR::eMailbox };
}

void VulkanApp::remakeSwapchain()
{
	try
	{
		swapchainData = makeSwapchain(false);
	}
	catch (const intvlk::SwapchainZeroDimensionError&)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
