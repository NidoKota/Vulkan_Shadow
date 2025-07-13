cmake_minimum_required(VERSION 3.22)

project(Vulkan_Test)

set(CMAKE_CXX_STANDARD 17)

add_custom_target(vertexshader ALL COMMAND "glslc" "../src/shader.vert" "-o" "../src/shader.vert.spv")
add_custom_target(fragmentshader ALL COMMAND "glslc" "../src/shader.frag" "-o" "../src/shader.frag.spv")
add_library(stb INTERFACE)
add_executable(app ../src/Main.cpp)

add_compile_definitions(VULKAN_TEST_MAC)

find_package(Vulkan REQUIRED)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(app PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(app PRIVATE ${Vulkan_LIBRARIES})

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(app PRIVATE glfw)