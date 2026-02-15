find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

add_library(intvlk)

target_sources(intvlk
    PRIVATE
        vma_utils/usage.cpp
    PUBLIC
        vma_utils/BufferData.hpp
        vma_utils/DepthAttachmentData.hpp
        vma_utils/ImageData.hpp
        vma_utils/MeshData.hpp
        vma_utils/usage.hpp
        vma_utils/utils.hpp
        Error.hpp
        SdlContext.hpp
        SwapchainData.hpp
        utils.hpp
        WindowData.hpp
)

target_include_directories(intvlk
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/external/VulkanMemoryAllocator/include
)

target_link_libraries(intvlk
    PUBLIC
        Vulkan::Vulkan
        SDL2::SDL2
)

target_compile_features(intvlk PUBLIC cxx_std_20)

set_target_properties(intvlk PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/lib
)

if(APPLE)
    target_compile_definitions(intvlk
        PUBLIC
            VK_USE_PLATFORM_METAL_EXT
            VK_ENABLE_BETA_EXTENSIONS
    )
endif()
