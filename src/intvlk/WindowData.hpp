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

#include "SdlContext.hpp"

#include <mutex>

namespace intvlk
{
    class WindowData
    {
    public:
        WindowData(std::string_view windowName, const vk::Extent2D& extent)
            : handle{ makeWindow(windowName, extent) },
            name{ windowName },
            extent{ extent }
        {
            SDL_AddEventWatch(windowResizeEventWatch, this);
        }

        static std::shared_ptr<SDL_Window> makeWindow(std::string_view windowName, const vk::Extent2D& extent)
        {
            SDL_WindowFlags windowFlags{ static_cast<SDL_WindowFlags>(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE) };
            SDL_Window* handle{ SDL_CreateWindow(windowName.data(),
                                                SDL_WINDOWPOS_CENTERED,
                                                SDL_WINDOWPOS_CENTERED,
                                                extent.width,
                                                extent.height,
                                                windowFlags) };
            if (!handle)
            {
                throw Error{ "Failed to create window!" };
            }
            return std::shared_ptr<SDL_Window>{handle, [](SDL_Window* window)
                {
                    if (window)
                    {
                        SDL_DestroyWindow(window);
                    }
                }};
        }

        static int windowResizeEventWatch(void* userData, SDL_Event* event)
        {
            if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_RESIZED)
            {
                auto windowData{ static_cast<WindowData*>(userData) };
                windowData->setExtent(vk::Extent2D{ static_cast<uint32_t>(event->window.data1),
                                                   static_cast<uint32_t>(event->window.data2) });
            }
            return 0;
        }

        vk::Extent2D getExtent()
        {
            std::lock_guard lock{ extentMutex };
            return extent;
        }

        std::string_view getName() const
        {
            return name;
        }

        void setExtent(const vk::Extent2D& newExtent)
        {
            std::lock_guard lock{ extentMutex };
            extent = newExtent;
        }

        void setName(std::string_view newName)
        {
            name = newName;
            SDL_SetWindowTitle(handle.get(), newName.data());
        }

        std::shared_ptr<SDL_Window> handle{};

    private:
        SdlContext sdlContext{};
        std::string name{};
        vk::Extent2D extent{};
        std::mutex extentMutex{};
    };
}
