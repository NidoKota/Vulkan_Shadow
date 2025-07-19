/*
 * Vulkanデバイスクラス
 *
 * 物理Vulkanデバイスとその論理表現をカプセル化します
 *
 * Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
// SRS - ベータ拡張を有効にし、VK_KHR_portability_subsetを可視にする
#define VK_ENABLE_BETA_EXTENSIONS
#endif
#include <VulkanDevice.h>
#include <unordered_set>

namespace vks
{
    /**
    * デフォルトコンストラクタ
    *
    * @param physicalDevice 使用される物理デバイス
    */
    VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice)
    {
        assert(physicalDevice);
        this->physicalDevice = physicalDevice;

        // 物理デバイスのプロパティ、機能、制限を後で使用するために保存する
        // デバイスプロパティには、制限（limits）とスパースプロパティも含まれる
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        // 機能（features）は、使用する前にサンプル側でチェックされるべき
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);
        // メモリプロパティは、あらゆる種類のバッファを作成するために定期的に使用される
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
        // キューファミリープロパティ。デバイス作成時に要求されたキューを設定するために使用される
        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        assert(queueFamilyCount > 0);
        queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        // サポートされている拡張機能のリストを取得する
        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
        if (extCount > 0)
        {
            std::vector<VkExtensionProperties> extensions(extCount);
            if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
            {
                for (auto& ext : extensions)
                {
                    supportedExtensions.push_back(ext.extensionName);
                }
            }
        }
    }

    /**
    * デフォルトデストラクタ
    *
    * @note 論理デバイスを解放します
    */
    VulkanDevice::~VulkanDevice()
    {
        if (commandPool)
        {
            vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
        }
        if (logicalDevice)
        {
            vkDestroyDevice(logicalDevice, nullptr);
        }
    }

    /**
    * 要求されたすべてのプロパティビットが設定されているメモリタイプのインデックスを取得します
    *
    * @param typeBits 要求するリソースがサポートする各メモリタイプに対応するビットが設定されたビットマスク（VkMemoryRequirementsから取得）
    * @param properties 要求するメモリタイプのプロパティのビットマスク
    * @param (任意) memTypeFound 一致するメモリタイプが見つかった場合にtrueに設定されるboolへのポインタ
    *
    * @return 要求されたメモリタイプのインデックス
    *
    * @throw memTypeFoundがnullで、要求されたプロパティをサポートするメモリタイプが見つからなかった場合に例外をスローします
    */
    uint32_t VulkanDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound) const
    {
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    if (memTypeFound)
                    {
                        *memTypeFound = true;
                    }
                    return i;
                }
            }
            typeBits >>= 1;
        }

        if (memTypeFound)
        {
            *memTypeFound = false;
            return 0;
        }
        else
        {
            throw std::runtime_error("Could not find a matching memory type");
        }
    }

    /**
    * 要求されたキューフラグをサポートするキューファミリーのインデックスを取得します
    * SRS - 単一フラグのみのVkQueueFlagBitsに対し、複数フラグを要求するためのVkQueueFlagsパラメータをサポート
    *
    * @param queueFlags キューファミリーインデックスを見つけるためのキューフラグ
    *
    * @return フラグに一致するキューファミリーのインデックス
    *
    * @throw 要求されたフラグをサポートするキューファミリーインデックスが見つからなかった場合に例外をスローします
    */
    uint32_t VulkanDevice::getQueueFamilyIndex(VkQueueFlags queueFlags) const
    {
        // コンピュート用の専用キュー
        // コンピュートをサポートし、グラフィックスをサポートしないキューファミリーインデックスを探す
        if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
            {
                if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    return i;
                }
            }
        }

        // 転送用の専用キュー
        // 転送をサポートし、グラフィックスとコンピュートをサポートしないキューファミリーインデックスを探す
        if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
            {
                if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                {
                    return i;
                }
            }
        }

        // 他のキュータイプの場合、または個別のコンピュートキューが存在しない場合は、要求されたフラグをサポートする最初のものを返す
        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
        {
            if ((queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags)
            {
                return i;
            }
        }

        throw std::runtime_error("Could not find a matching queue family index");
    }

    /**
    * 割り当てられた物理デバイスに基づいて論理デバイスを作成し、デフォルトのキューファミリーインデックスも取得します
    *
    * @param enabledFeatures デバイス作成時に特定の機能を有効化するために使用できます
    * @param pNextChain (任意) 拡張構造体へのポインタのチェーン
    * @param useSwapChain ヘッドなしレンダリングでスワップチェーンデバイス拡張を省略する場合はfalseに設定します
    * @param requestedQueueTypes デバイスに要求するキュータイプを指定するビットフラグ
    *
    * @return デバイス作成呼び出しのVkResult
    */
    VkResult VulkanDevice::createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char*> enabledExtensions, void* pNextChain, bool useSwapChain, VkQueueFlags requestedQueueTypes)
    {
        // 目的のキューは論理デバイス作成時に要求する必要がある
        // Vulkan実装のキューファミリー構成は様々であるため、特にアプリケーションが
        // 異なるキュータイプを要求する場合には、少し注意が必要になる

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

        // 要求されたキューファミリータイプに対応するキューファミリーインデックスを取得する
        // 実装によってはインデックスが重複する可能性があることに注意

        const float defaultQueuePriority(0.0f);

        // グラフィックスキュー
        if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
        {
            queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInfos.push_back(queueInfo);
        }
        else
        {
            queueFamilyIndices.graphics = 0;
        }

        // 専用のコンピュートキュー
        if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
        {
            queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
            if (queueFamilyIndices.compute != queueFamilyIndices.graphics)
            {
                // コンピュートファミリーのインデックスが異なる場合、コンピュートキュー用に追加のキュー作成情報が必要になる
                VkDeviceQueueCreateInfo queueInfo{};
                queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
                queueInfo.queueCount = 1;
                queueInfo.pQueuePriorities = &defaultQueuePriority;
                queueCreateInfos.push_back(queueInfo);
            }
        }
        else
        {
            // そうでなければ同じキューを使用する
            queueFamilyIndices.compute = queueFamilyIndices.graphics;
        }

        // 専用の転送キュー
        if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
        {
            queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
            if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) && (queueFamilyIndices.transfer != queueFamilyIndices.compute))
            {
                // 転送ファミリーのインデックスが異なる場合、転送キュー用に追加のキュー作成情報が必要になる
                VkDeviceQueueCreateInfo queueInfo{};
                queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
                queueInfo.queueCount = 1;
                queueInfo.pQueuePriorities = &defaultQueuePriority;
                queueCreateInfos.push_back(queueInfo);
            }
        }
        else
        {
            // そうでなければ同じキューを使用する
            queueFamilyIndices.transfer = queueFamilyIndices.graphics;
        }

        // 論理デバイス表現を作成する
        std::vector<const char*> deviceExtensions(enabledExtensions);
        if (useSwapChain)
        {
            // デバイスがスワップチェーンを介してディスプレイに表示するために使用される場合、スワップチェーン拡張を要求する必要がある
            deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

        // pNext(Chain)が渡された場合、それをデバイス作成情報に追加する必要がある
        VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
        if (pNextChain) {
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.features = enabledFeatures;
            physicalDeviceFeatures2.pNext = pNextChain;
            deviceCreateInfo.pEnabledFeatures = nullptr;
            deviceCreateInfo.pNext = &physicalDeviceFeatures2;
        }

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)) && defined(VK_KHR_portability_subset)
        // SRS - MoltenVKを使用したiOS/macOSで実行し、VK_KHR_portability_subsetが定義されデバイスでサポートされている場合、その拡張を有効にする
        if (extensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
        {
            deviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        }
#endif

        if (deviceExtensions.size() > 0)
        {
            for (const char* enabledExtension : deviceExtensions)
            {
                if (!extensionSupported(enabledExtension)) {
                    std::cerr << "Enabled device extension \"" << enabledExtension << "\" is not present at device level\n";
                }
            }

            deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
            deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        }

        this->enabledFeatures = enabledFeatures;

        VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
        if (result != VK_SUCCESS)
        {
            return result;
        }

        // グラフィックスコマンドバッファ用のデフォルトコマンドプールを作成する
        commandPool = createCommandPool(queueFamilyIndices.graphics);

        return result;
    }

    /**
    * デバイス上にバッファを作成します
    *
    * @param usageFlags バッファの用途フラグビットマスク（例：インデックス、頂点、ユニフォームバッファ）
    * @param memoryPropertyFlags このバッファのメモリプロパティ（例：デバイスローカル、ホスト可視、コヒーレント）
    * @param size バッファのサイズ（バイト単位）
    * @param buffer 関数によって取得されるバッファハンドルへのポインタ
    * @param memory 関数によって取得されるメモリハンドルへのポインタ
    * @param data 作成後にバッファにコピーされるデータへのポインタ（任意。設定されない場合、データはコピーされない）
    *
    * @return バッファハンドルとメモリが作成され、（任意で渡された）データがコピーされた場合にVK_SUCCESS
    */
    VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data)
    {
        // バッファハンドルを作成
        VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo(usageFlags, size);
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer));

        // バッファハンドルを裏付けるメモリを作成
        VkMemoryRequirements memReqs;
        VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
        vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        // バッファのプロパティに適合するメモリタイプのインデックスを見つける
        memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
        // バッファにVK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BITが設定されている場合、割り当て時に適切なフラグも有効にする必要がある
        VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
        if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
            allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            memAlloc.pNext = &allocFlagsInfo;
        }
        VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));

        // バッファデータへのポインタが渡された場合、バッファをマップしてデータをコピーする
        if (data != nullptr)
        {
            void *mapped;
            VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
            memcpy(mapped, data, size);
            // ホストコヒーレンシーが要求されていない場合、書き込みを可視にするために手動でフラッシュする
            if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange mappedRange = vks::initializers::mappedMemoryRange();
                mappedRange.memory = *memory;
                mappedRange.offset = 0;
                mappedRange.size = size;
                vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
            }
            vkUnmapMemory(logicalDevice, *memory);
        }

        // メモリをバッファオブジェクトにアタッチする
        VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

        return VK_SUCCESS;
    }

    /**
    * デバイス上にバッファを作成します
    *
    * @param usageFlags バッファの用途フラグビットマスク（例：インデックス、頂点、ユニフォームバッファ）
    * @param memoryPropertyFlags このバッファのメモリプロパティ（例：デバイスローカル、ホスト可視、コヒーレント）
    * @param buffer vks::Bufferオブジェクトへのポインタ
    * @param size バッファのサイズ（バイト単位）
    * @param data 作成後にバッファにコピーされるデータへのポインタ（任意。設定されない場合、データはコピーされない）
    *
    * @return バッファハンドルとメモリが作成され、（任意で渡された）データがコピーされた場合にVK_SUCCESS
    */
    VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, vks::Buffer *buffer, VkDeviceSize size, void *data)
    {
        buffer->device = logicalDevice;

        // バッファハンドルを作成
        VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo(usageFlags, size);
        VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer->buffer));

        // バッファハンドルを裏付けるメモリを作成
        VkMemoryRequirements memReqs;
        VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
        vkGetBufferMemoryRequirements(logicalDevice, buffer->buffer, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        // バッファのプロパティに適合するメモリタイプのインデックスを見つける
        memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
        // バッファにVK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BITが設定されている場合、割り当て時に適切なフラグも有効にする必要がある
        VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
        if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
            allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            memAlloc.pNext = &allocFlagsInfo;
        }
        VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &buffer->memory));

        buffer->alignment = memReqs.alignment;
        buffer->size = size;
        buffer->usageFlags = usageFlags;
        buffer->memoryPropertyFlags = memoryPropertyFlags;

        // バッファデータへのポインタが渡された場合、バッファをマップしてデータをコピーする
        if (data != nullptr)
        {
            VK_CHECK_RESULT(buffer->map());
            memcpy(buffer->mapped, data, size);
            if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
                buffer->flush();

            buffer->unmap();
        }

        // バッファ全体をカバーするデフォルトのディスクリプタを初期化する
        buffer->setupDescriptor();

        // メモリをバッファオブジェクトにアタッチする
        return buffer->bind();
    }

    /**
    * VkCmdCopyBufferを使用してバッファデータをsrcからdstにコピーします
    *
    * @param src コピー元のソースバッファへのポインタ
    * @param dst コピー先のデスティネーションバッファへのポインタ
    * @param queue キュー
    * @param copyRegion (任意) コピー領域へのポインタ。NULLの場合、バッファ全体がコピーされる
    *
    * @note ソースとデスティネーションのバッファは、適切な転送用途フラグ（TRANSFER_SRC / TRANSFER_DST）が設定されている必要があります
    */
    void VulkanDevice::copyBuffer(vks::Buffer *src, vks::Buffer *dst, VkQueue queue, VkBufferCopy *copyRegion)
    {
        assert(dst->size <= src->size);
        assert(src->buffer);
        VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        VkBufferCopy bufferCopy{};
        if (copyRegion == nullptr)
        {
            bufferCopy.size = src->size;
        }
        else
        {
            bufferCopy = *copyRegion;
        }

        vkCmdCopyBuffer(copyCmd, src->buffer, dst->buffer, 1, &bufferCopy);

        flushCommandBuffer(copyCmd, queue);
    }

    /**
    * コマンドバッファを割り当てるためのコマンドプールを作成します
    *
    * @param queueFamilyIndex コマンドプールを作成するキューのファミリーインデックス
    * @param createFlags (任意) コマンドプール作成フラグ（デフォルトは VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT）
    *
    * @note 作成されたプールから割り当てられたコマンドバッファは、同じファミリーインデックスを持つキューにのみサブミットできます
    *
    * @return 作成されたコマンドプールへのハンドル
    */
    VkCommandPool VulkanDevice::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
    {
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
        cmdPoolInfo.flags = createFlags;
        VkCommandPool cmdPool;
        VK_CHECK_RESULT(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool));
        return cmdPool;
    }

    /**
    * コマンドプールからコマンドバッファを割り当てます
    *
    * @param level 新しいコマンドバッファのレベル（プライマリまたはセカンダリ）
    * @param pool コマンドバッファが割り当てられるコマンドプール
    * @param (任意) begin trueの場合、新しいコマンドバッファでの記録が開始されます(vkBeginCommandBuffer)（デフォルトはfalse）
    *
    * @return 割り当てられたコマンドバッファへのハンドル
    */
    VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(pool, level, 1);
        VkCommandBuffer cmdBuffer;
        VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));
        // 要求された場合、新しいコマンドバッファの記録も開始する
        if (begin)
        {
            VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
            VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
        }
        return cmdBuffer;
    }

    VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, bool begin)
    {
        return createCommandBuffer(level, commandPool, begin);
    }

    /**
    * コマンドバッファの記録を終了し、キューにサブミットします
    *
    * @param commandBuffer フラッシュするコマンドバッファ
    * @param queue コマンドバッファをサブミットするキュー
    * @param pool コマンドバッファが作成されたコマンドプール
    * @param free (任意) コマンドバッファがサブミットされた後、解放します（デフォルトはtrue）
    *
    * @note コマンドバッファがサブミットされるキューは、それが割り当てられたプールと同じファミリーインデックスのものでなければなりません
    * @note フェンスを使用して、コマンドバッファの実行が完了したことを保証します
    */
    void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free)
    {
        if (commandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo = vks::initializers::submitInfo();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        // コマンドバッファの実行が完了したことを保証するためにフェンスを作成
        VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo(VK_FLAGS_NONE);
        VkFence fence;
        VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
        // キューにサブミット
        VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
        // フェンスがコマンドバッファの実行完了を通知するまで待機
        VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
        vkDestroyFence(logicalDevice, fence, nullptr);
        if (free)
        {
            vkFreeCommandBuffers(logicalDevice, pool, 1, &commandBuffer);
        }
    }

    void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
    {
        return flushCommandBuffer(commandBuffer, queue, commandPool, free);
    }

    /**
    * 拡張機能が（物理デバイスで）サポートされているか確認します
    *
    * @param extension 確認する拡張機能の名前
    *
    * @return 拡張機能がサポートされている（デバイス作成時に読み込まれたリストに存在する）場合はTrue
    */
    bool VulkanDevice::extensionSupported(std::string extension)
    {
        return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end());
    }

    /**
    * 使用可能なデプス（およびステンシル）フォーマットのリストから、このデバイスに最適なデプスフォーマットを選択します
    *
    * @param checkSamplingSupport フォーマットがサンプリング可能か（例：シェーダーでの読み取り）をチェックします
    *
    * @return 現在のデバイスに最も適合するデプスフォーマット
    *
    * @throw 要件に合うデプスフォーマットが見つからない場合に例外をスローします
    */
    VkFormat VulkanDevice::getSupportedDepthFormat(bool checkSamplingSupport)
    {
        // すべてのデプスフォーマットはオプショナルである可能性があるため、使用に適したデプスフォーマットを見つける必要がある
        std::vector<VkFormat> depthFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
        for (auto& format : depthFormats)
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
            // フォーマットはオプティマルタイリングのためにデプス・ステンシルアタッチメントをサポートしている必要がある
            if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                if (checkSamplingSupport) {
                    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
                        continue;
                    }
                }
                return format;
            }
        }
        throw std::runtime_error("Could not find a matching depth format");
    }

};