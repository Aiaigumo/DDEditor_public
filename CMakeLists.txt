cmake_minimum_required(VERSION 3.20)
project(DDEditor)

# Strict warnings 
option(DD_STRICT_WARNINGS "Enable strict compiler warnings" ON)

# Global settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL
find_package(OpenGL REQUIRED)

# GLFW
add_subdirectory(external/glfw)

# glad
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# ImGui
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw glad)

# miniaudio
add_library(miniaudio STATIC external/miniaudio/extras/stb_vorbis.c)
target_include_directories(miniaudio INTERFACE external/miniaudio)

# glm
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE external/glm)

# json
add_library(json INTERFACE)
target_include_directories(json INTERFACE external/json)

# Main executable
add_executable(DDEditor
    src/main.cpp
    src/AudioPlayer.cpp
    src/Timing.cpp
    src/TimingList.cpp
    src/Note.cpp
    src/Beatmap.cpp
    src/BeatmapInfo.cpp
    src/BeatmapStack.cpp
    src/Renderer.cpp
    src/InputState.cpp 
    src/Editor.cpp 
    src/EditorUtility.cpp 
    src/Application.cpp
    src/BeatmapInfoScreen.cpp
    src/DifficultyEditorScreen.cpp
    src/NativeFileDialog.cpp
    )

target_include_directories(DDEditor PRIVATE src)

target_link_libraries(DDEditor PRIVATE
    glfw
    glad
    imgui
    miniaudio
    glm
    json
    OpenGL::GL
)

# strict warnings
if (DD_STRICT_WARNINGS)

    if (MSVC)

        target_compile_options(DDEditor PRIVATE
            /W4            # High warning level
            /permissive-   # Strict standard conformance
            /Zc:__cplusplus
            /Zc:preprocessor
        )

    else()

        target_compile_options(DDEditor PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
        )

    endif()

endif()


# Copy resources for debug
# add_custom_command(
#     TARGET DDEditor POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#        "${CMAKE_SOURCE_DIR}/debugResources"
#         "${CMAKE_CURRENT_BINARY_DIR}"
# )

# Windows settings
if (WIN32)
    set_target_properties(DDEditor PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(DDEditor PRIVATE /ENTRY:mainCRTStartup)
    target_link_libraries(DDEditor PRIVATE ole32 shell32)
endif()

# Done
message(STATUS "DDEditor configured successfully")
