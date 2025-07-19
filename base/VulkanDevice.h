/*
 * Vulkanデバイスクラス
 *
 * 物理Vulkanデバイスとその論理表現をカプセル化します
 *
 * Copyright (C) 2016-2023 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "VulkanBuffer.h"
#include "VulkanTools.h"
#include "vulkan/vulkan.h"
#include <algorithm>
#include <assert.h>
#include <exception>

namespace vks
{
struct VulkanDevice
{
    /** @brief 物理デバイス表現 */
    VkPhysicalDevice physicalDevice;
    /** @brief 論理デバイス表現（アプリケーションから見たデバイス） */
    VkDevice logicalDevice;
    /** @brief 物理デバイスのプロパティ。アプリケーションが確認可能な制限（limits）を含みます */
    VkPhysicalDeviceProperties properties;
    /** @brief 物理デバイスの機能。アプリケーションが特定の機能がサポートされているか確認するために使用します */
    VkPhysicalDeviceFeatures features;
    /** @brief 物理デバイス上で使用するために有効化された機能 */
    VkPhysicalDeviceFeatures enabledFeatures;
    /** @brief 物理デバイスのメモリタイプとヒープ */
    VkPhysicalDeviceMemoryProperties memoryProperties;
    /** @brief 物理デバイスのキューファミリープロパティ */
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    /** @brief デバイスがサポートする拡張機能のリスト */
    std::vector<std::string> supportedExtensions;
    /** @brief グラフィックスキューファミリーインデックス用のデフォルトコマンドプール */
    VkCommandPool commandPool = VK_NULL_HANDLE;
    /** @brief キューファミリーのインデックスを格納します */
    struct
    {
        uint32_t graphics;
        uint32_t compute;
        uint32_t transfer;
    } queueFamilyIndices;
    operator VkDevice() const
    {
        return logicalDevice;
    };
    explicit VulkanDevice(VkPhysicalDevice physicalDevice);
    ~VulkanDevice();
    uint32_t        getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr) const;
    uint32_t        getQueueFamilyIndex(VkQueueFlags queueFlags) const;
    VkResult        createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char *> enabledExtensions, void *pNextChain, bool useSwapChain = true, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    VkResult        createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data = nullptr);
    VkResult        createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, vks::Buffer *buffer, VkDeviceSize size, void *data = nullptr);
    void            copyBuffer(vks::Buffer *src, vks::Buffer *dst, VkQueue queue, VkBufferCopy *copyRegion = nullptr);
    VkCommandPool   createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin = false);
    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin = false);
    void            flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true);
    void            flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);
    bool            extensionSupported(std::string extension);
    VkFormat        getSupportedDepthFormat(bool checkSamplingSupport);
};
} // namespace vks