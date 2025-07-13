# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

set(APP_NAME "vulkan_test")
project(${APP_NAME} VERSION 1.0 LANGUAGES CXX)

set(APPLICATION_SRC_DIR ${PROJECT_SOURCE_DIR}/../../../../../../Vulkan_Test)

set(CMAKE_CXX_STANDARD 17)

add_custom_target(vertexshader ALL COMMAND "glslc" "${APPLICATION_SRC_DIR}/src/shader.vert" "-o" "${PROJECT_SOURCE_DIR}/app/src/main/assets/shader.vert.spv")
add_custom_target(fragmentshader ALL COMMAND "glslc" "${APPLICATION_SRC_DIR}/src/shader.frag" "-o" "${PROJECT_SOURCE_DIR}/app/src/main/assets/shader.frag.spv")

#file(GLOB SOURCES "${APPLICATION_SRC_DIR}/src/*.cpp")
file(GLOB HEADERS "${APPLICATION_SRC_DIR}/include/*.hpp")

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(${APP_NAME} SHARED
        AndroidOut.cpp
        ${SOURCES}
        ${HEADERS}
        main.cpp
)

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

target_compile_definitions(${APP_NAME} PRIVATE VK_USE_PLATFORM_ANDROID)
target_compile_definitions(${APP_NAME} PRIVATE VK_USE_PLATFORM_ANDROID_KHR)
target_include_directories(${APP_NAME} PUBLIC
#        ${APPLICATION_SRC_DIR}/src
        ${APPLICATION_SRC_DIR}/include
        ${APPLICATION_SRC_DIR}/android/app/Vulkan-Hpp
        ${PROJECT_SOURCE_DIR}/ )

# Configure libraries CMake uses to link your target library.
target_link_libraries(${APP_NAME}
        # The game activity
        vulkan
        game-activity::game-activity
        android
        log)