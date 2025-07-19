/*
* Vulkanバッファクラス
*
* Vulkanバッファをカプセル化します
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanBuffer.h"

namespace vks
{   
    /** * このバッファのメモリ領域をマップします。成功した場合、`mapped`は指定されたバッファ領域を指します。
    * * @param size (任意) マップするメモリ領域のサイズ。バッファ全体の領域をマップするには`VK_WHOLE_SIZE`を渡します。
    * @param offset (任意) 先頭からのバイトオフセット
    * * @return バッファマッピング呼び出しの`VkResult`
    */
    VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset)
    {
        return vkMapMemory(device, memory, offset, size, 0, &mapped);
    }

    /**
    * マップされたメモリ領域をアンマップします
    *
    * @note `vkUnmapMemory`は失敗しないため、結果を返しません
    */
    void Buffer::unmap()
    {
        if (mapped)
        {
            vkUnmapMemory(device, memory);
            mapped = nullptr;
        }
    }

    /** * 割り当てられたメモリブロックをバッファにアタッチします
    * * @param offset (任意) バインドするメモリ領域の（先頭からの）バイトオフセット
    * * @return `bindBufferMemory`呼び出しの`VkResult`
    */
    VkResult Buffer::bind(VkDeviceSize offset)
    {
        return vkBindBufferMemory(device, buffer, memory, offset);
    }

    /**
    * このバッファのデフォルトディスクリプタを設定します
    *
    * @param size (任意) ディスクリプタのメモリ領域のサイズ
    * @param offset (任意) 先頭からのバイトオフセット
    *
    */
    void Buffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset)
    {
        descriptor.offset = offset;
        descriptor.buffer = buffer;
        descriptor.range = size;
    }

    /**
    * 指定されたデータをマップされたバッファにコピーします
    * * @param data コピーするデータへのポインタ
    * @param size コピーするデータのサイズ（バイト単位）
    *
    */
    void Buffer::copyTo(void* data, VkDeviceSize size)
    {
        assert(mapped);
        memcpy(mapped, data, size);
    }

    /** * バッファのメモリ領域をフラッシュして、デバイスから見えるようにします
    *
    * @note 非コヒーレントメモリの場合にのみ必要です
    *
    * @param size (任意) フラッシュするメモリ領域のサイズ。バッファ全体の領域をフラッシュするには`VK_WHOLE_SIZE`を渡します。
    * @param offset (任意) 先頭からのバイトオフセット
    *
    * @return フラッシュ呼び出しの`VkResult`
    */
    VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset)
    {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
    }

    /**
    * バッファのメモリ領域を無効化して、ホストから見えるようにします
    *
    * @note 非コヒーレントメモリの場合にのみ必要です
    *
    * @param size (任意) 無効化するメモリ領域のサイズ。バッファ全体の領域を無効化するには`VK_WHOLE_SIZE`を渡します。
    * @param offset (任意) 先頭からのバイトオフセット
    *
    * @return 無効化呼び出しの`VkResult`
    */
    VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
    {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
    }

    /** * このバッファが保持しているすべてのVulkanリソースを解放します
    */
    void Buffer::destroy()
    {
        if (buffer)
        {
            vkDestroyBuffer(device, buffer, nullptr);
        }
        if (memory)
        {
            vkFreeMemory(device, memory, nullptr);
        }
    }
};