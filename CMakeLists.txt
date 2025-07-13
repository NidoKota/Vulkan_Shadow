# CMakeの最低要求バージョンを設定
cmake_minimum_required(VERSION 3.22)

# vcpkgツールチェーンファイルへのパスを設定
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# プロジェクト名を設定
project(Vulkan_Shadow)

# C++の標準をC++20に設定
set(CMAKE_CXX_STANDARD 20)

# --- 必要なパッケージを検索 ---
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_compile_definitions(
    VULKAN_TEST_WIN
    VK_USE_PLATFORM_WIN32_KHR
    NOMINMAX
)

# --- グローバルなインクルードディレクトリ ---
# ここで指定したディレクトリは、このプロジェクトのすべてのターゲットに適用されます
include_directories(
    external
    external/imgui
    external/tinygltf
    external/ktx/include
    external/ktx/other_include
    base
)

# --- ヘルパーライブラリとサブディレクトリ ---
# 'base'サブディレクトリを追加 (baseライブラリがビルドされる)
add_subdirectory(base)

# stbライブラリをINTERFACEライブラリとして定義 (主にヘッダーオンリーライブラリで使用)
add_library(stb INTERFACE)

# --- 実行可能ターゲットの定義 ---
# 'Triangle.cpp'をソースとする'triangle_app'という名前の実行可能ファイルを定義
add_executable(triangle_app src/Triangle.cpp)

# 'Main.cpp'をソースとする'main_app'という名前の実行可能ファイルを定義
add_executable(main_app src/Main.cpp)

# --- 共通のターゲットプロパティを設定 ---
# 設定を適用するターゲットのリストを作成
set(ALL_APPS triangle_app main_app)

# foreachループを使用して、各ターゲットに共通の設定を適用
foreach(APP ${ALL_APPS})
    # 共通のインクルードディレクトリをターゲットに追加
    target_include_directories(${APP} PRIVATE
        ${Vulkan_INCLUDE_DIRS}
    )

    # 共通のライブラリをターゲットにリンク
    target_link_libraries(${APP} PRIVATE
        base
        stb
        ${Vulkan_LIBRARIES}
        glfw
        glm::glm
    )
endforeach()
