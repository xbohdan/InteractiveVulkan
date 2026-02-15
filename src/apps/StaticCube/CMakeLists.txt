add_library(glm INTERFACE)

target_include_directories(glm INTERFACE ${CMAKE_SOURCE_DIR}/external/glm)

target_compile_definitions(glm INTERFACE
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
)

make_vulkan_app(StaticCube EXTRA_LIBS glm)
make_shaders(StaticCube)