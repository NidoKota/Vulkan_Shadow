/*
* Vulkanサンプル - 基本的なインデックス付き三角形のレンダリング
*
* 注意：
*	これはVulkanを立ち上げて何かを表示する方法を示すための、いわば「ベタ踏み」のサンプルです。
*	他のサンプルとは対照的に、このサンプルでは（スワップチェーンのセットアップなどを除き）ヘルパー関数や初期化子は使用しません。
*
* Copyright (C) 2016-2023 by Sascha Willems - www.saschawillems.de
*
* このコードはMITライセンス（MIT）(http://opensource.org/licenses/MIT)の下でライセンスされています。
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <exception>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "../base/vulkanexamplebase.h"

// GPUとCPUを常にビジー状態に保ちたい。そのために、前のコマンドバッファがまだ実行中であっても、新しいコマンドバッファの構築を開始することがあります。
// この数は、同時に処理できるフレームの最大数を定義します。
// この数を増やすとパフォーマンスが向上する可能性がありますが、追加の遅延も発生します。
#define MAX_CONCURRENT_FRAMES 2

class VulkanExample : public VulkanExampleBase
{
public:
	// このサンプルで使用される頂点レイアウト
	struct Vertex {
		float position[3];
		float color[3];
	};

	// 頂点バッファと属性
	struct {
		VkDeviceMemory memory{ VK_NULL_HANDLE }; // このバッファのためのデバイスメモリへのハンドル
		VkBuffer buffer;						 // メモリがバインドされているVulkanバッファオブジェクトへのハンドル
	} vertices;

	// インデックスバッファ
	struct {
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer buffer;
		uint32_t count{ 0 };
	} indices;

	// ユニフォームバッファブロックオブジェクト
	struct UniformBuffer {
		VkDeviceMemory memory;
		VkBuffer buffer;
		// ディスクリプタセットは、シェーダーのバインディングポイントにバインドされたリソースを格納します。
		// これは、異なるシェーダーのバインディングポイントを、それらのバインディングに使用されるバッファやイメージに接続します。
		VkDescriptorSet descriptorSet;
		// マップされたバッファへのポインタを保持しておくことで、memcpyを介してその内容を簡単に更新できます。
		uint8_t* mapped{ nullptr };
	};
	// フレームごとに1つのUBOを使用することで、フレームのオーバーラップを可能にし、ユニフォームがまだ使用中に更新されないようにします。
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffers;

	// 簡単にするために、シェーダーと同じユニフォームブロックレイアウトを使用します：
	//
	//	layout(set = 0, binding = 0) uniform UBO
	//	{
	//		mat4 projectionMatrix;
	//		mat4 modelMatrix;
	//		mat4 viewMatrix;
	//	} ubo;
	//
	// このようにすることで、uboデータをそのままuboにmemcpyできます。
	// 注意：手動でのパディングを避けるために、GPUとアライメントが合うデータ型（vec4, mat4）を使用する必要があります。
	struct ShaderData {
		glm::mat4 projectionMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
	};

	// パイプラインレイアウトは、パイプラインがディスクリプタセットにアクセスするために使用されます。
	// これは、シェーダーステージとシェーダーリソース間のインターフェースを（実際のデータをバインドせずに）定義します。
	// パイプラインレイアウトは、インターフェースが一致する限り、複数のパイプライン間で共有できます。
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

	// パイプライン（しばしば「パイプラインステートオブジェクト」と呼ばれる）は、パイプラインに影響を与えるすべてのステートを「焼き付ける」ために使用されます。
	// OpenGLではすべてのステートが（ほぼ）いつでも変更できましたが、Vulkanではグラフィックス（およびコンピュート）パイプラインのステートを事前にレイアウトする必要があります。
	// そのため、動的でないパイプラインステートの組み合わせごとに、新しいパイプラインが必要になります（ここでは説明しないいくつかの例外があります）。
	// これは事前の計画という新たな次元を追加しますが、ドライバーによるパフォーマンス最適化の絶好の機会でもあります。
	VkPipeline pipeline{ VK_NULL_HANDLE };

	// ディスクリプタセットレイアウトは、シェーダーのバインディングレイアウトを（実際にディスクリプタを参照せずに）記述します。
	// パイプラインレイアウトと同様に、これはほぼ設計図のようなものであり、レイアウトが一致する限り、異なるディスクリプタセットで使用できます。
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };

	// 同期プリミティブ
	// 同期はVulkanの重要な概念であり、OpenGLではほとんど隠されていました。これを正しく行うことがVulkanを使用する上で非常に重要です。

	// セマフォは、グラフィックスキュー内の操作を調整し、正しいコマンド順序を保証するために使用されます。
	std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> presentCompleteSemaphores{};
	std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> renderCompleteSemaphores{};

	VkCommandPool commandPool{ VK_NULL_HANDLE };
	std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES> commandBuffers{};
	std::array<VkFence, MAX_CONCURRENT_FRAMES> waitFences{};

	// 正しい同期オブジェクトを選択するために、現在のフレームを追跡する必要があります。
	uint32_t currentFrame{ 0 };

	VulkanExample() : VulkanExampleBase()
	{
		title = "Vulkan Example - Basic indexed triangle";
		// 簡単にするために、フレームワークのUIオーバーレイは使用しません。
		settings.overlay = false;
		// デフォルトのlook-atカメラをセットアップします。
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));
		camera.setRotation(glm::vec3(0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 1.0f, 256.0f);
		// ここで設定されていない値は、基底クラスのコンストラクタで初期化されます。
	}

	~VulkanExample()
	{
		// 使用したVulkanリソースをクリーンアップします。
		// 注意：継承されたデストラクタが基底クラスに格納されているリソースをクリーンアップします。
		vkDestroyPipeline(device, pipeline, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, vertices.buffer, nullptr);
		vkFreeMemory(device, vertices.memory, nullptr);

		vkDestroyBuffer(device, indices.buffer, nullptr);
		vkFreeMemory(device, indices.memory, nullptr);

		vkDestroyCommandPool(device, commandPool, nullptr);

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			vkDestroyFence(device, waitFences[i], nullptr);
			vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
			vkDestroyBuffer(device, uniformBuffers[i].buffer, nullptr);
			vkFreeMemory(device, uniformBuffers[i].memory, nullptr);
		}
	}

	// この関数は、要求するすべてのプロパティフラグ（例：デバイスローカル、ホスト可視）をサポートするデバイスメモリタイプを要求するために使用されます。
	// 成功すると、要求されたメモリプロパティに適合するメモリタイプのインデックスを返します。
	// これは、実装が異なるメモリプロパティを持つ任意の数のメモリタイプを提供する可能性があるため、必要です。
	// さまざまなメモリ構成の詳細については、https://vulkan.gpuinfo.org/ を確認してください。
	uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
	{
		// このサンプルで使用されているデバイスで利用可能なすべてのメモリタイプをイテレートします。
		for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}
			typeBits >>= 1;
		}

		throw "Could not find a suitable memory type!";
	}

	// このサンプルで使用されるフレームごとの（in flight）Vulkan同期プリミティブを作成します。
	void createSynchronizationPrimitives()
	{
		// セマフォは、キュー内での正しいコマンド順序のために使用されます。
		VkSemaphoreCreateInfo semaphoreCI{};
		semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// フェンスは、ホスト側で描画コマンドバッファの完了を確認するために使用されます。
		VkFenceCreateInfo fenceCI{};
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// フェンスをシグナル状態で作成します（これにより、各コマンドバッファの最初のレンダリングで待機しなくなります）。
		fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			// 再度サブミットを開始する前に、イメージの提示が完了していることを保証するために使用されるセマフォ。
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &presentCompleteSemaphores[i]));
			// イメージをキューにサブミットする前に、サブミットされたすべてのコマンドが終了したことを保証するために使用されるセマフォ。
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &renderCompleteSemaphores[i]));

			// コマンドバッファを再度使用する前に、その実行が完了したことを保証するために使用されるフェンス。
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &waitFences[i]));
		}
	}

	void createCommandBuffers()
	{
		// すべてのコマンドバッファは、コマンドプールから割り当てられます。
		VkCommandPoolCreateInfo commandPoolCI{};
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCI.queueFamilyIndex = swapChain.queueNodeIndex;
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));

		// 上記のプールから、最大同時実行フレーム数ごとに1つのコマンドバッファを割り当てます。
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_CONCURRENT_FRAMES);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, commandBuffers.data()));
	}

	// インデックス付き三角形のための頂点およびインデックスバッファを準備します。
	// また、ステージングを使用してそれらをデバイスローカルメモリにアップロードし、頂点シェーダーに一致するように頂点入力と属性バインディングを初期化します。
	void createVertexBuffer()
	{
		// Vulkanのメモリ管理全般に関する注意点：
		//	これは非常に複雑なトピックであり、サンプルアプリケーションでは小さな個別のメモリアロケーションで問題ありませんが、
		//	実際のアプリケーションで行うべきことではありません。実際のアプリケーションでは、一度に大きなメモリチャンクを割り当てるべきです。

		// 頂点の設定
		std::vector<Vertex> vertexBuffer{
			{ { 1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
		};
		uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

		// インデックスの設定
		std::vector<uint32_t> indexBuffer{ 0, 1, 2 };
		indices.count = static_cast<uint32_t>(indexBuffer.size());
		uint32_t indexBufferSize = indices.count * sizeof(uint32_t);

		VkMemoryAllocateInfo memAlloc{};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs;

		// 頂点バッファやインデックスバッファのような静的データは、GPUによる最適（かつ最速）なアクセスのためにデバイスメモリに格納されるべきです。
		//
		// これを実現するために、いわゆる「ステージングバッファ」を使用します：
		// - ホストから可視な（そしてマップ可能な）バッファを作成する
		// - データをこのバッファにコピーする
		// - デバイス上に同じサイズのローカルなバッファ（VRAM）をもう一つ作成する
		// - コマンドバッファを使用してホストからデバイスへデータをコピーする
		// - ホスト可視の（ステージング）バッファを削除する
		// - レンダリングにはデバイスローカルのバッファを使用する
		//
		// 注意：ホスト（CPU）とGPUが同じメモリを共有する統合メモリアーキテクチャでは、ステージングは必要ありません。
		// このサンプルを分かりやすく保つため、そのチェックは行っていません。

		struct StagingBuffer {
			VkDeviceMemory memory;
			VkBuffer buffer;
		};

		struct {
			StagingBuffer vertices;
			StagingBuffer indices;
		} stagingBuffers;

		void* data;

		// 頂点バッファ
		VkBufferCreateInfo vertexBufferInfoCI{};
		vertexBufferInfoCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfoCI.size = vertexBufferSize;
		// バッファはコピー元として使用されます。
		vertexBufferInfoCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// 頂点データをコピーするためのホスト可視バッファ（ステージングバッファ）を作成します。
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &stagingBuffers.vertices.buffer));
		vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		// データをコピーするために使用できるホスト可 sichtなメモリタイプを要求します。
		// また、バッファのアンマップ直後に書き込みがGPUに見えるように、コヒーレントであることも要求します。
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));
		// マップしてコピー
		VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(device, stagingBuffers.vertices.memory);
		VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));

		// （ホストローカルな）頂点データがコピーされ、レンダリングに使用されるデバイスローカルなバッファを作成します。
		vertexBufferInfoCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &vertices.buffer));
		vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

		// インデックスバッファ
		VkBufferCreateInfo indexbufferCI{};
		indexbufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferCI.size = indexBufferSize;
		indexbufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// インデックスデータをホスト可視のバッファ（ステージングバッファ）にコピーします。
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &stagingBuffers.indices.buffer));
		vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.indices.memory));
		VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &data));
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(device, stagingBuffers.indices.memory);
		VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0));

		// デバイスのみ可視の宛先バッファを作成します。
		indexbufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &indices.buffer));
		vkGetBufferMemoryRequirements(device, indices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &indices.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, indices.buffer, indices.memory, 0));

		// バッファコピーはキューにサブミットする必要があるため、そのためのコマンドバッファが必要です。
		// 注意：一部のデバイスは、大量のコピーを行う際に高速になる可能性のある専用の転送キュー（転送ビットのみが設定されている）を提供しています。
		VkCommandBuffer copyCmd;

		VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = commandPool;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
		// バッファ領域のコピーをコマンドバッファに入れます。
		VkBufferCopy copyRegion{};
		// 頂点バッファ
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vertices.buffer, 1, &copyRegion);
		// インデックスバッファ
		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffers.indices.buffer, indices.buffer, 1, &copyRegion);
		VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

		// コピーを完了するために、コマンドバッファをキューにサブミットします。
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyCmd;

		// コマンドバッファの実行が完了したことを保証するためのフェンスを作成します。
		VkFenceCreateInfo fenceCI{};
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.flags = 0;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));

		// キューにサブミットします。
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		// フェンスがコマンドバッファの実行完了を通知するのを待ちます。
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

		vkDestroyFence(device, fence, nullptr);
		vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);

		// ステージングバッファを破棄します。
		// 注意：ステージングバッファは、コピーがサブミットされ実行される前に削除してはいけません。
		vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
		vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
		vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
		vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);
	}

	// ディスクリプタはプールから割り当てられます。このプールは、使用するディスクリプタの種類と（最大）数を実装に伝えます。
	void createDescriptorPool()
	{
		// APIに対して、型ごとに要求されるディスクリプタの最大数を伝える必要があります。
		VkDescriptorPoolSize descriptorTypeCounts[1];
		// このサンプルでは、ディスクリプタの型は1つ（ユニフォームバッファ）だけです。
		descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// フレームごとに1つのバッファ（つまり1つのディスクリプタ）があります。
		descriptorTypeCounts[0].descriptorCount = MAX_CONCURRENT_FRAMES;
		// 他の型を追加するには、型カウントリストに新しいエントリを追加する必要があります。
		// 例：2つの結合イメージサンプラーの場合：
		// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// typeCounts[1].descriptorCount = 2;

		// グローバルなディスクリプタプールを作成します。
		// このサンプルで使用されるすべてのディスクリプタは、このプールから割り当てられます。
		VkDescriptorPoolCreateInfo descriptorPoolCI{};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.pNext = nullptr;
		descriptorPoolCI.poolSizeCount = 1;
		descriptorPoolCI.pPoolSizes = descriptorTypeCounts;
		// このプールから要求できるディスクリプタセットの最大数を設定します（この制限を超えて要求するとエラーになります）。
		// このサンプルでは、フレームごとにユニフォームバッファごとに1つのセットを作成します。
		descriptorPoolCI.maxSets = MAX_CONCURRENT_FRAMES;
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));
	}

	// ディスクリプタセットレイアウトは、アプリケーションとシェーダー間のインターフェースを定義します。
	// 基本的に、異なるシェーダーステージを、ユニフォームバッファやイメージサンプラーなどをバインドするためのディスクリプタに接続します。
	// したがって、すべてのシェーダーバインディングは、1つのディスクリプタセットレイアウトバインディングにマッピングされるべきです。
	void createDescriptorSetLayout()
	{
		// バインディング 0: ユニフォームバッファ（頂点シェーダー）
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		layoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCI.pNext = nullptr;
		descriptorLayoutCI.bindingCount = 1;
		descriptorLayoutCI.pBindings = &layoutBinding;
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));

		// このディスクリプタセットレイアウトに基づいてレンダリングパイプラインを生成するために使用されるパイプラインレイアウトを作成します。
		// より複雑なシナリオでは、再利用可能な異なるディスクリプタセットレイアウトに対して、異なるパイプラインレイアウトを持つことになります。
		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.pNext = nullptr;
		pipelineLayoutCI.setLayoutCount = 1;
		pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));
	}

	// シェーダーは、ユニフォームバッファを「指す」ディスクリプタセットを使用してデータにアクセスします。
	// ディスクリプタセットは、上記で作成したディスクリプタセットレイアウトを利用します。
	void createDescriptorSets()
	{
		// グローバルなディスクリプタプールから、フレームごとに1つのディスクリプタセットを割り当てます。
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &descriptorSetLayout;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffers[i].descriptorSet));

			// シェーダーのバインディングポイントを決定するディスクリプタセットを更新します。
			// シェーダーで使用されるすべてのバインディングポイントに対して、そのバインディングポイントに一致するディスクリプタセットが1つ必要です。
			VkWriteDescriptorSet writeDescriptorSet{};

			// バッファの情報は、ディスクリプタ情報構造体を使用して渡されます。
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i].buffer;
			bufferInfo.range = sizeof(ShaderData);

			// バインディング 0 : ユニフォームバッファ
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = uniformBuffers[i].descriptorSet;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.pBufferInfo = &bufferInfo;
			writeDescriptorSet.dstBinding = 0;
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	// フレームバッファで使用される深度（およびステンシル）バッファアタッチメントを作成します。
	// 注意：基底クラスの仮想関数のオーバーライドであり、VulkanExampleBase::prepare内から呼び出されます。
	void setupDepthStencil()
	{
		// 深度ステンシルアタッチメントとして使用される最適なイメージを作成します。
		VkImageCreateInfo imageCI{};
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = depthFormat;
		// サンプルの幅と高さを使用します。
		imageCI.extent = { width, height, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));

		// イメージのためのメモリを（デバイスローカルに）割り当て、それを我々のイメージにバインドします。
		VkMemoryAllocateInfo memAlloc{};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &depthStencil.memory));
		VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0));

		// 深度ステンシルイメージのためのビューを作成します。
		// Vulkanではイメージは直接アクセスされず、サブリソース範囲によって記述されたビューを介してアクセスされます。
		// これにより、異なる範囲を持つ1つのイメージの複数のビューが可能になります（例：異なるレイヤーのため）。
		VkImageViewCreateInfo depthStencilViewCI{};
		depthStencilViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthStencilViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilViewCI.format = depthFormat;
		depthStencilViewCI.subresourceRange = {};
		depthStencilViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		// ステンシルアスペクトは、深度+ステンシルフォーマット（VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT）でのみ設定する必要があります。
		if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
			depthStencilViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		depthStencilViewCI.subresourceRange.baseMipLevel = 0;
		depthStencilViewCI.subresourceRange.levelCount = 1;
		depthStencilViewCI.subresourceRange.baseArrayLayer = 0;
		depthStencilViewCI.subresourceRange.layerCount = 1;
		depthStencilViewCI.image = depthStencil.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilViewCI, nullptr, &depthStencil.view));
	}

	// 各スワップチェーンイメージに対してフレームバッファを作成します。
	// 注意：基底クラスの仮想関数のオーバーライドであり、VulkanExampleBase::prepare内から呼び出されます。
	void setupFrameBuffer()
	{
		// スワップチェーン内のすべてのイメージに対してフレームバッファを作成します。
		frameBuffers.resize(swapChain.imageCount);
		for (size_t i = 0; i < frameBuffers.size(); i++)
		{
			std::array<VkImageView, 2> attachments;
			// カラーアタッチメントはスワップチェーンイメージのビューです。
			attachments[0] = swapChain.buffers[i].view;
			// 深度/ステンシルアタッチメントは、現在のGPUでの深度の動作方法のため、すべてのフレームバッファで同じです。
			attachments[1] = depthStencil.view;

			VkFramebufferCreateInfo frameBufferCI{};
			frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			// すべてのフレームバッファは同じレンダーパス設定を使用します。
			frameBufferCI.renderPass = renderPass;
			frameBufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
			frameBufferCI.pAttachments = attachments.data();
			frameBufferCI.width = width;
			frameBufferCI.height = height;
			frameBufferCI.layers = 1;
			// フレームバッファを作成します。
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCI, nullptr, &frameBuffers[i]));
		}
	}

	// レンダーパスの設定
	// レンダーパスはVulkanの新しい概念です。レンダリング中に使用されるアタッチメントを記述し、アタッチメントの依存関係を持つ複数のサブパスを含むことができます。
	// これにより、ドライバーはレンダリングがどのようになるかを事前に知ることができ、特にタイルベースのレンダラー（複数のサブパスを持つ）での最適化の良い機会となります。
	// サブパスの依存関係を使用すると、使用されるアタッチメントの暗黙的なレイアウト遷移も追加されるため、それらを変換するための明示的なイメージメモリバリアを追加する必要はありません。
	// 注意：基底クラスの仮想関数のオーバーライドであり、VulkanExampleBase::prepare内から呼び出されます。
	void setupRenderPass()
	{
		// このサンプルでは、1つのサブパスを持つ単一のレンダーパスを使用します。

		// このレンダーパスで使用されるアタッチメントの記述子。
		std::array<VkAttachmentDescription, 2> attachments{};

		// カラーアタッチメント
		attachments[0].format = swapChain.colorFormat;                  // スワップチェーンで選択されたカラーフォーマットを使用します。
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;                  // このサンプルではマルチサンプリングは使用しません。
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;             // レンダーパスの開始時にこのアタッチメントをクリアします。
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;           // レンダーパス終了後もその内容を保持します（表示のため）。
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // ステンシルは使用しないので、ロードは気にしません。
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // ストアも同様です。
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;        // レンダーパス開始時のレイアウト。初期レイアウトは重要ではないので、undefinedを使用します。
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;    // レンダーパス終了時にアタッチメントが遷移するレイアウト。
		// カラーバッファをスワップチェーンに提示したいため、PRESENT_KHRに遷移します。
// 深度アタッチメント
		attachments[1].format = depthFormat;                           // 適切な深度フォーマットがサンプル基底クラスで選択されます。
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;             // 最初のサブパスの開始時に深度をクリアします。
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;       // レンダーパス終了後に深度は不要です（DONT_CAREはパフォーマンス向上につながる可能性があります）。
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // ステンシルなし。
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // ステンシルなし。
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;        // レンダーパス開始時のレイアウト。初期レイアウトは重要ではないので、undefinedを使用します。
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // 深度/ステンシルアタッチメントに遷移します。

		// アタッチメント参照の設定
		VkAttachmentReference colorReference{};
		colorReference.attachment = 0;                                   // アタッチメント0はカラーです。
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // サブパス中にカラーとして使用されるアタッチメントレイアウト。

		VkAttachmentReference depthReference{};
		depthReference.attachment = 1;                                     // アタッチメント1は深度です。
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // サブパス中に深度/ステンシルとして使用されるアタッチメント。

		// 単一のサブパス参照の設定
		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;                     // サブパスは1つのカラーアタッチメントを使用します。
		subpassDescription.pColorAttachments = &colorReference;          // スロット0のカラーアタッチメントへの参照。
		subpassDescription.pDepthStencilAttachment = &depthReference;    // スロット1の深度アタッチメントへの参照。
		subpassDescription.inputAttachmentCount = 0;                     // 入力アタッチメントは、前のサブパスの内容からサンプリングするために使用できます。
		subpassDescription.pInputAttachments = nullptr;                  // (このサンプルでは入力アタッチメントは使用しません)
		subpassDescription.preserveAttachmentCount = 0;                  // 保持アタッチメントは、サブパス間でアタッチメントをループ（および保持）するために使用できます。
		subpassDescription.pPreserveAttachments = nullptr;               // (このサンプルでは保持アタッチメントは使用しません)
		subpassDescription.pResolveAttachments = nullptr;                // 解決アタッチメントはサブパスの最後に解決され、マルチサンプリングなどに使用できます。

		// サブパス依存関係の設定
		// これらは、アタッチメント記述で指定された暗黙的なアタッチメントレイアウト遷移を追加します。
		// 実際の使用レイアウトは、アタッチメント参照で指定されたレイアウトを通じて保持されます。
		// 各サブパス依存関係は、srcStageMask, dstStageMask, srcAccessMask, dstAccessMaskによって記述されるソースサブパスとデスティネーションサブパスの間にメモリおよび実行の依存関係を導入します（そしてdependencyFlagsが設定されます）。
		// 注意：VK_SUBPASS_EXTERNALは、実際のレンダーパスの外部で実行されるすべてのコマンドを参照する特別な定数です。
		std::array<VkSubpassDependency, 2> dependencies;

		// 深度およびカラーアタッチメントのfinalからinitialへのレイアウト遷移を行います。
		// 深度アタッチメント
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dependencies[0].dependencyFlags = 0;
		// カラーアタッチメント
		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = 0;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[1].dependencyFlags = 0;

		// 実際のレンダーパスを作成します。
		VkRenderPassCreateInfo renderPassCI{};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size()); // このレンダーパスで使用されるアタッチメントの数。
		renderPassCI.pAttachments = attachments.data();                           // レンダーパスで使用されるアタッチメントの記述。
		renderPassCI.subpassCount = 1;                                           // このサンプルでは1つのサブパスのみ使用します。
		renderPassCI.pSubpasses = &subpassDescription;                           // そのサブパスの記述。
		renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size()); // サブパス依存関係の数。
		renderPassCI.pDependencies = dependencies.data();                         // レンダーパスで使用されるサブパス依存関係。
		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderPass));
	}

	// Vulkanは、SPIR-Vと呼ばれる中間バイナリ表現からシェーダーをロードします。
	// シェーダーは、例えばGLSLから参照glslangコンパイラを使用してオフラインでコンパイルされます。
	// この関数は、そのようなシェーダーをバイナリファイルからロードし、シェーダーモジュール構造体を返します。
	VkShaderModule loadSPIRVShader(std::string filename)
	{
		size_t shaderSize;
		char* shaderCode{ nullptr };

#if defined(__ANDROID__)
		// 圧縮されたアセットからシェーダーをロードします。
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
		assert(asset);
		shaderSize = AAsset_getLength(asset);
		assert(shaderSize > 0);

		shaderCode = new char[shaderSize];
		AAsset_read(asset, shaderCode, shaderSize);
		AAsset_close(asset);
#else
		std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

		if (is.is_open())
		{
			shaderSize = is.tellg();
			is.seekg(0, std::ios::beg);
			// ファイルの内容をバッファにコピーします。
			shaderCode = new char[shaderSize];
			is.read(shaderCode, shaderSize);
			is.close();
			assert(shaderSize > 0);
		}
#endif
		if (shaderCode)
		{
			// パイプライン作成に使用される新しいシェーダーモジュールを作成します。
			VkShaderModuleCreateInfo shaderModuleCI{};
			shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleCI.codeSize = shaderSize;
			shaderModuleCI.pCode = (uint32_t*)shaderCode;

			VkShaderModule shaderModule;
			VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));

			delete[] shaderCode;

			return shaderModule;
		}
		else
		{
			std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
			return VK_NULL_HANDLE;
		}
	}

	void createPipelines()
	{
		// このサンプルで使用されるグラフィックスパイプラインを作成します。
		// Vulkanはレンダリングパイプラインの概念を使用して固定ステートをカプセル化し、OpenGLの複雑なステートマシンを置き換えます。
		// パイプラインはGPU上に格納されハッシュ化されるため、パイプラインの変更は非常に高速です。
		// 注意：パイプラインに直接含まれない動的なステートがいくつかまだ存在します（ただし、それらが使用されるという情報は含まれます）。

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		// このパイプラインに使用されるレイアウト（同じレイアウトを使用する複数のパイプライン間で共有可能）。
		pipelineCI.layout = pipelineLayout;
		// このパイプラインがアタッチされるレンダーパス。
		pipelineCI.renderPass = renderPass;

		// パイプラインを構成するさまざまなステートを構築します。

		// 入力アセンブリステートは、プリミティブがどのように組み立てられるかを記述します。
		// このパイプラインは頂点データをトライアングルリストとして組み立てます（ただし、1つのトライアングルしか使用しません）。
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// ラスタライゼーションステート
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCI.depthClampEnable = VK_FALSE;
		rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateCI.depthBiasEnable = VK_FALSE;
		rasterizationStateCI.lineWidth = 1.0f;

		// カラーブレンドステートは、（使用されている場合）ブレンド係数がどのように計算されるかを記述します。
		// （ブレンディングが使用されていなくても）カラーアタッチメントごとに1つのブレンドアタッチメントステートが必要です。
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = 0xf;
		blendAttachmentState.blendEnable = VK_FALSE;
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentState;

		// ビューポートステートは、このパイプラインで使用されるビューポートとシザーの数を設定します。
		// 注意：これは実際には動的ステートによって上書きされます（下記参照）。
		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;

		// 動的ステートの有効化
		// ほとんどのステートはパイプラインに焼き付けられますが、コマンドバッファ内で変更できる動的なステートもいくつかあります。
		// これらを変更できるようにするには、このパイプラインでどの動的ステートが変更されるかを指定する必要があります。実際のステートは後でコマンドバッファで設定されます。
		// このサンプルでは、ビューポートとシザーを動的ステートを使用して設定します。
		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// 深度およびステンシルの比較とテスト操作を含む、深度およびステンシルステート。
		// 深度テストのみを使用し、深度テストと書き込みを有効にし、less or equalで比較します。
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_TRUE;
		depthStencilStateCI.depthWriteEnable = VK_TRUE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
		depthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilStateCI.stencilTestEnable = VK_FALSE;
		depthStencilStateCI.front = depthStencilStateCI.back;

		// マルチサンプリングステート
		// このサンプルではマルチサンプリング（アンチエイリアシング用）を使用しませんが、ステートは設定してパイプラインに渡す必要があります。
		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleStateCI.pSampleMask = nullptr;

		// 頂点入力の記述
		// パイプラインの頂点入力パラメータを指定します。

		// 頂点入力バインディング
		// このサンプルでは、バインディングポイント0で単一の頂点入力バインディングを使用します（vkCmdBindVertexBuffersを参照）。
		VkVertexInputBindingDescription vertexInputBinding{};
		vertexInputBinding.binding = 0;
		vertexInputBinding.stride = sizeof(Vertex);
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// 入力属性バインディングは、シェーダー属性の位置とメモリレイアウトを記述します。
		std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs;
		// これらは次のシェーダーレイアウトに一致します（triangle.vertを参照）：
		//	layout (location = 0) in vec3 inPos;
		//	layout (location = 1) in vec3 inColor;
		// 属性位置 0: 位置
		vertexInputAttributs[0].binding = 0;
		vertexInputAttributs[0].location = 0;
		// 位置属性は3つの32ビット符号付き浮動小数点数（SFLOAT）です（R32 G32 B32）。
		vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributs[0].offset = offsetof(Vertex, position);
		// 属性位置 1: 色
		vertexInputAttributs[1].binding = 0;
		vertexInputAttributs[1].location = 1;
		// 色属性は3つの32ビット符号付き浮動小数点数（SFLOAT）です（R32 G32 B32）。
		vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributs[1].offset = offsetof(Vertex, color);

		// パイプライン作成に使用される頂点入力ステート。
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
		vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCI.vertexBindingDescriptionCount = 1;
		vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
		vertexInputStateCI.vertexAttributeDescriptionCount = 2;
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributs.data();

		// シェーダー
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		// 頂点シェーダー
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// このシェーダーのパイプラインステージを設定します。
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		// バイナリのSPIR-Vシェーダーをロードします。
		shaderStages[0].module = loadSPIRVShader(getShadersPath() + "triangle/triangle.vert.spv");
		// シェーダーのメインエントリーポイント。
		shaderStages[0].pName = "main";
		assert(shaderStages[0].module != VK_NULL_HANDLE);

		// フラグメントシェーダー
		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// このシェーダーのパイプラインステージを設定します。
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		// バイナリのSPIR-Vシェーダーをロードします。
		shaderStages[1].module = loadSPIRVShader(getShadersPath() + "triangle/triangle.frag.spv");
		// シェーダーのメインエントリーポイント。
		shaderStages[1].pName = "main";
		assert(shaderStages[1].module != VK_NULL_HANDLE);

		// パイプラインシェーダーステージ情報を設定します。
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		// パイプラインステートをパイプライン作成情報構造体に割り当てます。
		pipelineCI.pVertexInputState = &vertexInputStateCI;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;

		// 指定されたステートを使用してレンダリングパイプラインを作成します。
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

		// グラフィックスパイプラインが作成されると、シェーダーモジュールは不要になります。
		vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
		vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
	}

	void createUniformBuffers()
	{
		// シェーダーユニフォームを含むフレームごとのユニフォームバッファブロックを準備・初期化します。
		// OpenGLのような単一のユニフォームはVulkanにはもはや存在しません。すべてのシェーダーユニフォームはユニフォームバッファブロックを介して渡されます。
		VkMemoryRequirements memReqs;

		// 頂点シェーダーのユニフォームバッファブロック
		VkBufferCreateInfo bufferInfo{};
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = 0;
		allocInfo.memoryTypeIndex = 0;

		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(ShaderData);
		// このバッファはユニフォームバッファとして使用されます。
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		// バッファを作成します。
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffers[i].buffer));
			// サイズ、アライメント、メモリタイプを含むメモリ要件を取得します。
			vkGetBufferMemoryRequirements(device, uniformBuffers[i].buffer, &memReqs);
			allocInfo.allocationSize = memReqs.size;
			// ホスト可視のメモリアクセスをサポートするメモリタイプのインデックスを取得します。
			// ほとんどの実装は複数のメモリタイプを提供しており、メモリを割り当てるために正しいものを選択することが重要です。
			// また、バッファがホストコヒーレントであることを望みます。そうすれば、更新のたびにフラッシュ（または同期）する必要がありません。
			// 注意：これはパフォーマンスに影響を与える可能性があるため、定期的にバッファを更新する実際のアプリケーションでは行いたくないかもしれません。
			allocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			// ユニフォームバッファのためのメモリを割り当てます。
			VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &(uniformBuffers[i].memory)));
			// メモリをバッファにバインドします。
			VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffers[i].buffer, uniformBuffers[i].memory, 0));
			// バッファを一度マップしておくことで、再度マップすることなく更新できます。
			VK_CHECK_RESULT(vkMapMemory(device, uniformBuffers[i].memory, 0, sizeof(ShaderData), 0, (void**)&uniformBuffers[i].mapped));
		}

	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		createSynchronizationPrimitives();
		createCommandBuffers();
		createVertexBuffer();
		createUniformBuffers();
		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSets();
		createPipelines();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;

		// フェンスを使用して、コマンドバッファを再度使用する前にその実行が完了するのを待ちます。
		vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX);
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentFrame]));

		// 実装から次のスワップチェーンイメージを取得します。
		// 実装は任意の順序でイメージを返すことができるため、acquire関数を使用する必要があり、単にイメージ/imageIndexを自分でループすることはできません。
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			windowResize();
			return;
		}
		else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)) {
			throw "Could not acquire the next swap chain image!";
		}

		// 次のフレームのためにユニフォームバッファを更新します。
		ShaderData shaderData{};
		shaderData.projectionMatrix = camera.matrices.perspective;
		shaderData.viewMatrix = camera.matrices.view;
		shaderData.modelMatrix = glm::mat4(1.0f);

		// 現在の行列を現在のフレームのユニフォームバッファにコピーします。
		// 注意：ユニフォームバッファにホストコヒーレントなメモリタイプを要求したため、書き込みは即座にGPUに可視になります。
		memcpy(uniformBuffers[currentFrame].mapped, &shaderData, sizeof(ShaderData));

		// コマンドバッファを構築します。
		// OpenGLとは異なり、すべてのレンダリングコマンドはコマンドバッファに記録され、その後キューにサブミットされます。
		// これにより、別のスレッドで事前に作業を生成できます。
		// （このサンプルのような）基本的なコマンドバッファでは、記録が非常に高速なため、これをオフロードする必要はありません。

		vkResetCommandBuffer(commandBuffers[currentFrame], 0);

		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		// loadOpがclearに設定されているすべてのフレームバッファアタッチメントのクリア値を設定します。
		// 2つのアタッチメント（カラーと深度）を使用し、これらはサブパスの開始時にクリアされるため、両方のクリア値を設定する必要があります。
		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = frameBuffers[imageIndex];

		const VkCommandBuffer commandBuffer = commandBuffers[currentFrame];
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

		// 基底クラスによってデフォルトのレンダーパス設定で指定された最初のサブパスを開始します。
		// これにより、カラーと深度のアタッチメントがクリアされます。
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		// 動的なビューポートステートを更新します。
		VkViewport viewport{};
		viewport.height = (float)height;
		viewport.width = (float)width;
		viewport.minDepth = (float)0.0f;
		viewport.maxDepth = (float)1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		// 動的なシザーステートを更新します。
		VkRect2D scissor{};
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		// 現在のフレームのユニフォームバッファのディスクリプタセットをバインドし、この描画でシェーダーがそのバッファのデータを使用するようにします。
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uniformBuffers[currentFrame].descriptorSet, 0, nullptr);
		// レンダリングパイプラインをバインドします。
		// パイプライン（ステートオブジェクト）にはレンダリングパイプラインのすべてのステートが含まれており、これをバインドするとパイプライン作成時に指定されたすべてのステートが設定されます。
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// 三角形の頂点バッファ（位置と色を含む）をバインドします。
		VkDeviceSize offsets[1]{ 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
		// 三角形のインデックスバッファをバインドします。
		vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		// インデックス付き三角形を描画します。
		vkCmdDrawIndexed(commandBuffer, indices.count, 1, 0, 0, 1);
		vkCmdEndRenderPass(commandBuffer);
		// レンダーパスを終了すると、フレームバッファのカラーアタッチメントをウィンドウシステムに提示するためにVK_IMAGE_LAYOUT_PRESENT_SRC_KHRに遷移させる暗黙のバリアが追加されます。
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		// コマンドバッファをグラフィックスキューにサブミットします。

		// キューのサブミッションが（pWaitSemaphoresを介して）待機するパイプラインステージ。
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		// submit info構造体は、コマンドバッファのキューサブミッションバッチを指定します。
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &waitStageMask;       // セマフォの待機が発生するパイプラインステージのリストへのポインタ。
		submitInfo.pCommandBuffers = &commandBuffer;		// このバッチ（サブミッション）で実行するコマンドバッファ。
		submitInfo.commandBufferCount = 1;                   // 単一のコマンドバッファをサブミットします。

		// サブミットされたコマンドバッファが実行を開始する前に待機するセマフォ。
		submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentFrame];
		submitInfo.waitSemaphoreCount = 1;
		// コマンドバッファが完了したときにシグナルされるセマフォ。
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentFrame];
		submitInfo.signalSemaphoreCount = 1;

		// 待機フェンスを渡してグラフィックスキューにサブミットします。
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentFrame]));

		// 現在のフレームバッファをスワップチェーンに提示します。
		// コマンドバッファのサブミッションによってシグナルされたセマフォを、スワップチェーン提示の待機セマフォとして渡します。
		// これにより、すべてのコマンドがサブミットされるまで、イメージがウィンドウシステムに提示されないことが保証されます。

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphores[currentFrame];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain.swapChain;
		presentInfo.pImageIndices = &imageIndex;
		result = vkQueuePresentKHR(queue, &presentInfo);

		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
			windowResize();
		}
		else if (result != VK_SUCCESS) {
			throw "Could not present the image to the swap chain!";
		}

		// 最大同時実行フレーム数に基づいて、次にレンダリングするフレームを選択します。
		currentFrame = (currentFrame + 1) % MAX_CONCURRENT_FRAMES;
	}
};

// OS固有のメインエントリーポイント
// コードベースのほとんどは、サポートされているさまざまなオペレーティングシステムで共有されていますが、メッセージ処理などは異なります。

#if defined(_WIN32)
// Windowsのエントリーポイント
VulkanExample* vulkanExample;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
	for (size_t i = 0; i < __argc; i++) { VulkanExample::args.push_back(__argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow(hInstance, WndProc);
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}

#elif defined(__ANDROID__)
// Androidのエントリーポイント
VulkanExample* vulkanExample;
void android_main(android_app* state)
{
	vulkanExample = new VulkanExample();
	state->userData = vulkanExample;
	state->onAppCmd = VulkanExample::handleAppCommand;
	state->onInputEvent = VulkanExample::handleAppInput;
	androidApp = state;
	vulkanExample->renderLoop();
	delete(vulkanExample);
}
#elif defined(_DIRECT2DISPLAY)

// Direct to display wsi を使用した Linux のエントリーポイント
// Direct to Displays (D2D) は組み込みプラットフォームで使用されます。
VulkanExample* vulkanExample;
static void handleEvent()
{
}
int main(const int argc, const char* argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
VulkanExample* vulkanExample;
static void handleEvent(const DFBWindowEvent* event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
int main(const int argc, const char* argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
VulkanExample* vulkanExample;
int main(const int argc, const char* argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(__linux__) || defined(__FreeBSD__)

// Linuxのエントリーポイント
VulkanExample* vulkanExample;
#if defined(VK_USE_PLATFORM_XCB_KHR)
static void handleEvent(const xcb_generic_event_t* event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
#else
static void handleEvent()
{
}
#endif
int main(const int argc, const char* argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif (defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)) && defined(VK_EXAMPLE_XCODE_GENERATED)
VulkanExample* vulkanExample;
int main(const int argc, const char* argv[])
{
	@autoreleasepool
	{
		for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
		vulkanExample = new VulkanExample();
		vulkanExample->initVulkan();
		vulkanExample->setupWindow(nullptr);
		vulkanExample->prepare();
		vulkanExample->renderLoop();
		delete(vulkanExample);
	}
	return 0;
}
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
VULKAN_EXAMPLE_MAIN()
#endif