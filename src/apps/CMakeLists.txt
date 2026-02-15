function(make_vulkan_app APP_NAME)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs EXTRA_LIBS)
    cmake_parse_arguments(APP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    file(GLOB_RECURSE APP_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hh"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    )
    list(FILTER APP_SRC EXCLUDE REGEX ".*/shaders/.*")

    add_executable(${APP_NAME} ${APP_SRC})

    target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(${APP_NAME} PRIVATE intvlk ${APP_EXTRA_LIBS})

    target_compile_features(${APP_NAME} PRIVATE cxx_std_20)

    set_target_properties(${APP_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/bin/${APP_NAME}
    )
endfunction()

function(make_shaders TARGET)
    find_program(GLSL_VALIDATOR NAMES glslangValidator REQUIRED)

    set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

    file(GLOB SHADER_SRC "${SHADER_SRC_DIR}/*.*")

    set(SHADER_OUT_DIR $<TARGET_FILE_DIR:${TARGET}>/shaders)

    foreach(SHADER_FILE IN LISTS SHADER_SRC)
        get_filename_component(FILE_NAME ${SHADER_FILE} NAME)

        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}

            COMMAND ${GLSL_VALIDATOR} -V ${SHADER_FILE} -o ${SHADER_OUT_DIR}/${FILE_NAME}.spv

            COMMENT "Compiling shader ${FILE_NAME}..."
            VERBATIM
        )
    endforeach()
endfunction()

add_subdirectory(BitGenerator)
add_subdirectory(HammingNeighbors)
add_subdirectory(StaticCube)