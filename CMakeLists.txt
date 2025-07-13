cmake_minimum_required(VERSION 3.22)

set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(Vulkan_Shadow)

set(CMAKE_CXX_STANDARD 20)

include_directories(external)
include_directories(external/imgui)
include_directories(external/tinygltf)
include_directories(external/ktx/include)
include_directories(external/ktx/other_include)
include_directories(base)

add_library(stb INTERFACE)
add_executable(app "../src/Main.cpp")

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_compile_definitions(VULKAN_TEST_WIN)
add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
add_compile_definitions(NOMINMAX)

target_include_directories(app PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(app PRIVATE
    base
    stb
    ${Vulkan_LIBRARIES}
    glfw
    glm::glm
)

add_subdirectory(base)