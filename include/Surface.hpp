#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#if !defined(__ANDROID__)
#include <GLFW/glfw3.h>
#endif
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

// 表示する先の画面は「サーフェス」というオブジェクトで抽象化されている
// 関連付けられた描画可能な対象 (サーフェス) がどのような特性を持っているかを Vulkan に知らせる
// これには、サポートされているピクセルフォーマット、色空間、プレゼンテーションモード (垂直同期の有無など)、現在のサイズなどが含まれる
// ウィンドウやスクリーンなど、「表示する先として使える何か」は全てサーフェスという同じ種類のオブジェクトで表され、統一的に扱うことができる
//
// これを通してVulkanはウィンドウや画面に描画できるようになる
// 描画結果を実際に画面に表示するためには、vk::SurfaceKHR と互換性のあるスワップチェーン (vk::SwapchainKHR) を作成する必要がある
// スワップチェーンは、描画に使用する複数のイメージ (通常はダブルバッファリングやトリプルバッファリング) を管理し、描画が終わったイメージをサーフェスに提示 (present) する役割を担う
#if defined(__ANDROID__)
std::shared_ptr<vk::UniqueSurfaceKHR> getSurface(vk::UniqueInstance& instance, ANativeWindow* window)
{
    std::shared_ptr<vk::UniqueSurfaceKHR> result = std::make_shared<vk::UniqueSurfaceKHR>();
    vk::AndroidSurfaceCreateInfoKHR surfaceCreateInfo(vk::AndroidSurfaceCreateFlagsKHR(), window);
    *result = instance->createAndroidSurfaceKHRUnique(surfaceCreateInfo);
    return result;
}
#else
std::shared_ptr<vk::UniqueSurfaceKHR> getSurface(vk::UniqueInstance& instance, GLFWwindow& window)
{
    VkSurfaceKHR c_surface;
    VkResult result = glfwCreateWindowSurface(*instance, &window, nullptr, &c_surface);
    if (result != VK_SUCCESS) 
    {
        const char* err;
        glfwGetError(&err);
        LOG(err);
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    return std::make_shared<vk::UniqueSurfaceKHR>(c_surface, *instance);
}
#endif

// 「物理デバイスが対象のサーフェスを扱う能力」の情報を取得する
std::shared_ptr<std::vector<vk::SurfaceFormatKHR>> getSurfaceFormats(vk::PhysicalDevice& physicalDevice, vk::UniqueSurfaceKHR& surface)
{
    std::shared_ptr<std::vector<vk::SurfaceFormatKHR>> result = std::make_shared<std::vector<vk::SurfaceFormatKHR>>();
    *result = physicalDevice.getSurfaceFormatsKHR(surface.get());
    return result;
}

// 「サーフェスの情報」の情報を取得する
std::shared_ptr<vk::SurfaceCapabilitiesKHR> getSurfaceCapabilities(vk::PhysicalDevice& physicalDevice, vk::UniqueSurfaceKHR& surface)
{
    std::shared_ptr<vk::SurfaceCapabilitiesKHR> result = std::make_shared<vk::SurfaceCapabilitiesKHR>();

    *result = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
    return result;
}

// 「物理デバイスが対象のサーフェスを扱う能力」の情報を取得する
std::shared_ptr<std::vector<vk::PresentModeKHR>> getSurfacePresentModes(vk::PhysicalDevice& physicalDevice, vk::UniqueSurfaceKHR& surface)
{
    std::shared_ptr<std::vector<vk::PresentModeKHR>> result = std::make_shared<std::vector<vk::PresentModeKHR>>();
    *result = physicalDevice.getSurfacePresentModesKHR(surface.get());
    return result;
}
