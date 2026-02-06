#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace apps::hamming_neighbors
{
    class SortPushConstants
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
}
