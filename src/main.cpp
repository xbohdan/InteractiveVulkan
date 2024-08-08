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

#include <iostream>

int main(int /*argc*/, char** /*argv*/)
{
    try
    {
        const std::string appName{ "Vulkan App" };
        const uint32_t width{ 900 };
        const uint32_t height{ 600 };
        VulkanApp app{ appName, width, height };
        app.run();
    }
    catch (const intvlk::Error& e)
    {
        std::cerr << "intvlk::Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (const vk::Error& e)
    {
        std::cerr << "vk::Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        std::cerr << "std::exception: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "Unknown error!\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}