# CMakeの最低要求バージョンを設定
cmake_minimum_required(VERSION 3.22)

IF(WIN32)
    # vcpkgツールチェーンファイルへのパスを設定
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
ENDIF()

# プロジェクト名を設定
project(Vulkan_Shadow)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

include_directories(
    external/stb
    external/imgui
    external/tinygltf
    external/ktx/include
    external/ktx/other_include
    base
)

add_subdirectory(base)

IF(WIN32)
    target_compile_definitions(base PUBLIC 
        VK_USE_PLATFORM_WIN32_KHR 
        NOMINMAX
    )
ELSEIF(APPLE)
    target_compile_definitions(base PUBLIC 
        VK_USE_PLATFORM_METAL_EXT
        VK_EXAMPLE_XCODE_GENERATED
    )
ENDIF()

set(ALL_APPS Triangle Main)

foreach(APP ${ALL_APPS})
    add_executable(${APP}App src/${APP}.cpp)

    IF(APPLE)
        # ターゲットごとにObjective-C++コンパイルオプションを追加
        IF(CMAKE_GENERATOR MATCHES "Makefiles" OR CMAKE_GENERATOR MATCHES "Ninja")
            target_compile_options(${APP} PRIVATE -fobjc-arc -ObjC++)
        ELSE() # Xcodeの場合
            target_compile_options(${APP} PRIVATE -fobjc-arc -xobjective-c++)
        ENDIF()
        
        target_link_libraries(${APP} PRIVATE
            "-framework AppKit"
            "-framework QuartzCore"
        )
    ENDIF()

    # 共通のインクルードディレクトリをターゲットに追加
    target_include_directories(${APP}App PRIVATE
        ${Vulkan_INCLUDE_DIRS}
    )

    # 共通のライブラリをターゲットにリンク
    target_link_libraries(${APP}App PRIVATE
        base
        ${Vulkan_LIBRARIES}
        glfw
        glm::glm
    )
endforeach()
