#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

std::shared_ptr<std::vector<vk::PhysicalDevice>> getPhysicalDevices(vk::UniqueInstance& instance)
{
    // vk::Instanceにはそれに対応するvk::UniqueInstanceが存在したが、
    // vk::PhysicalDeviceに対応するvk::UniquePhysicalDevice は存在しない
    // destroyなどを呼ぶ必要もない
    // vk::PhysicalDeviceは単に物理的なデバイスの情報を表しているに過ぎないので、構築したり破棄したりする必要がある類のオブジェクトではない

    std::shared_ptr<std::vector<vk::PhysicalDevice>> result = std::make_shared<std::vector<vk::PhysicalDevice>>();
    *result = instance.get().enumeratePhysicalDevices();
    return result;
}

std::shared_ptr<std::pair<vk::PhysicalDevice, uint32_t>> selectPhysicalDeviceAndQueueFamilyIndex(vk::PhysicalDevice& physicalDevice, uint32_t queueFamilyIndex)
{
    std::shared_ptr<std::pair<vk::PhysicalDevice, uint32_t>> result;

    std::vector<vk::ExtensionProperties> extProps = physicalDevice.enumerateDeviceExtensionProperties();
    for (size_t i = 0; i < extProps.size(); i++) 
    {
        // enumerateDeviceExtensionPropertiesメソッドでその物理デバイスがサポートしている拡張機能の一覧を取得
        // その中にスワップチェーンの拡張機能の名前が含まれていなければそのデバイスは使わない
        if (std::string_view(extProps[i].extensionName.data()) != VK_KHR_SWAPCHAIN_EXTENSION_NAME) 
        {
            continue;
        }

        result.reset(new std::pair<vk::PhysicalDevice, uint32_t>{physicalDevice, queueFamilyIndex});
        break;
    }

    return result;
}

std::shared_ptr<std::pair<vk::PhysicalDevice, uint32_t>> selectPhysicalDeviceAndQueueFamilyIndex(vk::PhysicalDevice& physicalDevice, vk::UniqueSurfaceKHR& surface)
{
    std::shared_ptr<std::pair<vk::PhysicalDevice, uint32_t>> result;

    // Vulkanにおけるキューとは、GPUの実行するコマンドを保持する待ち行列 = GPUのやることリスト
    // GPUにコマンドを送るときは、このキューにコマンドを詰め込むことになる
    // 
    // 1つのGPUが持っているキューは1つだけとは限らない
    // キューによってサポートしている機能とサポートしていない機能がある
    // キューにコマンドを送るときは、そのキューが何の機能をサポートしているかを事前に把握しておく必要がある
    // 
    // ある物理デバイスの持っているキューの情報は getQueueFamilyProperties メソッドで取得できる
    // メソッド名にある「キューファミリ」というのは、同じ能力を持っているキューをひとまとめにしたもの
    // 1つの物理デバイスには1個以上のキューファミリがあり、1つのキューファミリには1個以上の同等の機能を持ったキューが所属している
    // 1つのキューファミリの情報は vk::QueueFamilyProperties 構造体で表される
    std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
    for (size_t i = 0; i < queueProps.size(); i++)
    {
        // グラフィックス機能に加えてサーフェスへのプレゼンテーションもサポートしているキューを厳選
        if (!(queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics) || !physicalDevice.getSurfaceSupportKHR(i, *surface))
        {
            continue;
        }
        
        result = selectPhysicalDeviceAndQueueFamilyIndex(physicalDevice, i);
        if (result != nullptr)
        {
            break;
        }
    }
    
    return result;
}

std::shared_ptr<std::pair<vk::PhysicalDevice, uint32_t>> selectPhysicalDeviceAndQueueFamilyIndex(std::vector<vk::PhysicalDevice>& physicalDevices, vk::UniqueSurfaceKHR& surface)
{
    // GPUが複数あるなら頼む相手をまず選ぶ必要がある
    // GPUの型式などによってサポートしている機能とサポートしていない機能があったりするため、
    // だいたい「インスタンスを介して物理デバイスを列挙する」→「それぞれの物理デバイスの情報を取得する」→「一番いいのを頼む」という流れになる

    std::shared_ptr<std::pair<vk::PhysicalDevice, uint32_t>> result;

    // デバイスとキューの検索
    for (size_t i = 0; i < physicalDevices.size(); i++)
    {
        vk::PhysicalDevice& physicalDevice = physicalDevices[i];

        // デバイスがサーフェスを間違いなくサポートしていることを確かめる
        if (physicalDevice.getSurfaceFormatsKHR(*surface).empty() && physicalDevice.getSurfacePresentModesKHR(*surface).empty())
        {
            continue;
        }

        result = selectPhysicalDeviceAndQueueFamilyIndex(physicalDevice, surface);
        if (result != nullptr)
        {
            break;
        }
    }

    if (result == nullptr)
    {
        LOGERR("No physical devices are available");
        exit(EXIT_FAILURE);
    }

    return result;
}