cmake_minimum_required(VERSION 3.22)

project(Vulkan_Shadow)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

include_directories(
    external
    external/stb
    external/imgui
    external/tinygltf
    external/ktx/include
    external/ktx/other_include
    base
)

add_subdirectory(base)

add_executable(triangle_app src/Triangle.cpp)
add_executable(main_app src/Main.cpp)

set(ALL_APPS triangle_app main_app)
# 'base' ライブラリもプラットフォーム設定が必要なため、新しいリストを作成します
set(ALL_VULKAN_TARGETS base ${ALL_APPS})

# ------------------------------------------------------------------
# ↓↓↓ すべてのVulkanターゲットにプラットフォーム固有のコンパイル設定を適用します ↓↓↓
# ------------------------------------------------------------------
foreach(TARGET ${ALL_VULKAN_TARGETS})
    # --- プラットフォームごとの設定 ---
    IF(APPLE)
        # ターゲットごとにプラットフォーム定義を追加
        target_compile_definitions(${TARGET} PRIVATE
            VK_USE_PLATFORM_METAL_EXT
            VK_EXAMPLE_XCODE_GENERATED # 必要に応じて
        )

        # ターゲットごとにObjective-C++コンパイルオプションを追加
        IF(CMAKE_GENERATOR MATCHES "Makefiles" OR CMAKE_GENERATOR MATCHES "Ninja")
            target_compile_options(${TARGET} PRIVATE -fobjc-arc -ObjC++)
        ELSE() # Xcodeの場合
            target_compile_options(${TARGET} PRIVATE -fobjc-arc -xobjective-c++)
        ENDIF()

    ELSEIF(WIN32)
        target_compile_definitions(${TARGET} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
        # 他のWindows用設定...
    ELSE() # Linuxなど
        target_compile_definitions(${TARGET} PRIVATE VK_USE_PLATFORM_XCB_KHR)
        # 他のLinux用設定...
    ENDIF()
endforeach()


# ------------------------------------------------------------------
# ↓↓↓ 各アプリケーションのリンク設定を行います ↓↓↓
# ------------------------------------------------------------------
foreach(APP ${ALL_APPS})
    # 共通でリンクするライブラリ
    target_link_libraries(${APP} PRIVATE
        base
        ${Vulkan_LIBRARIES}
        glfw
        glm::glm
    )

    # 共通でインクルードするディレクトリ
    target_include_directories(${APP} PRIVATE
        ${Vulkan_INCLUDE_DIRS}
    )

    # Appleプラットフォームでのみフレームワークをリンク
    IF(APPLE)
        target_link_libraries(${APP} PRIVATE
            "-framework AppKit"
            "-framework QuartzCore"
        )
    ENDIF()
endforeach()
