/*
* Vulkanバッファクラス
*
* Vulkanバッファをカプセル化します
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vector>

#include "vulkan/vulkan.h"
#include "VulkanTools.h"

namespace vks
{   
    /**
    * @brief デバイスメモリに裏付けされたVulkanバッファへのアクセスをカプセル化します
    * @note VulkanDeviceのような外部ソースによって設定されることを想定しています
    */
    struct Buffer
    {
        VkDevice device;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDescriptorBufferInfo descriptor;
        VkDeviceSize size = 0;
        VkDeviceSize alignment = 0;
        void* mapped = nullptr;
        /** @brief バッファ作成時に外部ソースによって設定される用途フラグ（後でクエリするために使用） */
        VkBufferUsageFlags usageFlags;
        /** @brief バッファ作成時に外部ソースによって設定されるメモリプロパティフラグ（後でクエリするために使用） */
        VkMemoryPropertyFlags memoryPropertyFlags;
        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void unmap();
        VkResult bind(VkDeviceSize offset = 0);
        void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void copyTo(void* data, VkDeviceSize size);
        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void destroy();
    };
}