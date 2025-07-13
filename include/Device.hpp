#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

std::shared_ptr<std::vector<float>> getQueuePriorities()
{    
    // 今のところ欲しいキューは1つだけなので要素数1の配列にする
    std::shared_ptr<std::vector<float>> result = std::make_shared<std::vector<float>>();
    result->push_back(1.0f);
    return result;
}

std::shared_ptr<std::vector<vk::DeviceQueueCreateInfo>> getDeviceQueueCreateInfos(std::vector<float>& queuePriorities, uint32_t queueFamilyIndex)
{ 
    // queueFamilyIndex はキューの欲しいキューファミリのインデックスを表し、 
    // queueCount はそのキューファミリからいくつのキューが欲しいかを表す
    // 今欲しいのはグラフィック機能をサポートするキューを1つだけ
    // pQueuePriorities はキューのタスク実行の優先度を表すfloat値配列を指定
    // 優先度の値はキューごとに決められるため、この配列はそのキューファミリから欲しいキューの数だけの要素数を持ちます
    
    std::shared_ptr<std::vector<vk::DeviceQueueCreateInfo>> result = std::make_shared<std::vector<vk::DeviceQueueCreateInfo>>();
    result->push_back(vk::DeviceQueueCreateInfo());

    (*result)[0].queueFamilyIndex = queueFamilyIndex;
    (*result)[0].queueCount = queuePriorities.size();
    (*result)[0].pQueuePriorities = queuePriorities.data();

    return result;
}

std::shared_ptr<std::vector<const char*>> getRequiredLayers()
{
    // インスタンスを作成するときにvk::InstanceCreateInfo構造体を使ったのと同じように、
    // 論理デバイス作成時にもvk::DeviceCreateInfo構造体の中に色々な情報を含めることができる
    std::shared_ptr<std::vector<const char*>> result = std::make_shared<std::vector<const char*>>();
    result->push_back("VK_LAYER_KHRONOS_validation");
    return result;
}

std::shared_ptr<std::vector<const char*>> getRequiredExtensions()
{
    // スワップチェーンは拡張機能なので、機能を有効化する必要がある
    // スワップチェーンは「デバイスレベル」の拡張機能
    std::shared_ptr<std::vector<const char*>> result = std::make_shared<std::vector<const char*>>();
    result->push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    return result;
}

std::shared_ptr<vk::DeviceCreateInfo> getDeviceCreateInfo(std::vector<const char*>& deviceRequiredLayers, std::vector<const char*>& deviceRequiredExtensions, std::vector<vk::DeviceQueueCreateInfo>& queueCreateInfo)
{
    std::shared_ptr<vk::DeviceCreateInfo> result = std::make_shared<vk::DeviceCreateInfo>();
    result->enabledLayerCount = deviceRequiredLayers.size();
    result->ppEnabledLayerNames = deviceRequiredLayers.data();
    result->enabledExtensionCount = deviceRequiredExtensions.size();
    result->ppEnabledExtensionNames = deviceRequiredExtensions.data();
    result->queueCreateInfoCount = queueCreateInfo.size();
    result->pQueueCreateInfos = queueCreateInfo.data();
    return result;
}

std::shared_ptr<vk::UniqueDevice> getDevice(vk::PhysicalDevice& physicalDevice, vk::DeviceCreateInfo& deviceCreateInfo)
{
    // GPUが複数あるなら頼む相手をまず選ぶ
    // ここで言う「選ぶ」とは、特定のGPUを完全に占有してしまうとかそういう話ではない
    // 
    // 物理デバイスを選んだら次は論理デバイスを作成する
    // VulkanにおいてGPUの能力を使うような機能は、全てこの論理デバイスを通して利用する
    // GPUの能力を使う際、物理デバイスを直接いじることは出来ない
    // 
    // コンピュータの仕組みについて多少知識のある人であれば、
    // この「物理」「論理」の区別はメモリアドレスの「物理アドレス」「論理アドレス」と同じ話であることが想像できる
    // マルチタスクOS上で、1つのプロセスが特定のGPU(=物理デバイス)を独占して管理するような状態はよろしくない
    // 
    // 仮想化されたデバイスが論理デバイス
    // これならあるプロセスが他のプロセスの存在を意識することなくGPUの能力を使うことができる

    std::shared_ptr<vk::UniqueDevice> result = std::make_shared<vk::UniqueDevice>();
    *result = physicalDevice.createDeviceUnique(deviceCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDevice> getDevice(vk::PhysicalDevice& physicalDevice, uint32_t queueFamilyIndex)
{
    std::shared_ptr<std::vector<float>> queuePriorities = getQueuePriorities();
    std::shared_ptr<std::vector<vk::DeviceQueueCreateInfo>> deviceQueueCreateInfos = getDeviceQueueCreateInfos(*queuePriorities, queueFamilyIndex);

    std::shared_ptr<std::vector<const char*>> deviceRequiredLayers = getRequiredLayers();
    std::shared_ptr<std::vector<const char*>> deviceRequiredExtensions = getRequiredExtensions();

    std::shared_ptr<vk::DeviceCreateInfo> deviceCreateInfo = getDeviceCreateInfo(*deviceRequiredLayers, *deviceRequiredExtensions, *deviceQueueCreateInfos);

    return getDevice(physicalDevice, *deviceCreateInfo);
}
