#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

std::shared_ptr<vk::UniqueCommandPool> getCommandPool(vk::UniqueDevice& device, uint32_t queueFamilyIndex)
{
    std::shared_ptr<vk::UniqueCommandPool> result = std::make_shared<vk::UniqueCommandPool>();
    
    // コマンドバッファを作るには、その前段階として「コマンドプール」というまた別のオブジェクトを作る必要がある
    // コマンドバッファをコマンドの記録に使うオブジェクトとすれば、コマンドプールというのはコマンドを記録するためのメモリ実体
    // コマンドバッファを作る際には必ず必要
    // コマンドプールは論理デバイス(vk::Device)の createCommandPool メソッド、コマンドバッファは論理デバイス(vk::Device)の allocateCommandBuffersメソッドで作成することができる
    // コマンドプールの作成が「create」なのに対し、コマンドバッファの作成は「allocate」であるあたりから
    // 「コマンドバッファの記録能力は既に用意したコマンドプールから割り当てる」という気持ちが読み取れる

    // コマンドバッファとは、コマンドをため込んでおくバッファ
    // VulkanでGPUに仕事をさせる際は「コマンドバッファの中にコマンドを記録し、そのコマンドバッファをキューに送る」必要がある
    vk::CommandPoolCreateInfo cmdPoolCreateInfo;

    // CommandPoolCreateInfoのqueueFamilyIndexには、後でそのコマンドバッファを送信するときに対象とするキューを指定する
    // 結局送信するときにも「このコマンドバッファをこのキューに送信する」というのは指定するが、こんな二度手間が盛り沢山なのがVulkanである
    cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

    // もしも固定の内容のコマンドバッファを使ってしまうと、毎回同じフレームバッファが使われることになる
    // 毎回同じフレームバッファということは、毎回同じイメージに向けて描画することになってしまう
    // これではスワップチェーンによる表示処理がうまくいかない
    // そこで毎フレームコマンドバッファをリセットして、コマンドを記録し直す こうすることで毎回別のイメージに向けて描画できる
    // コマンドプール作成時 vk::CommandPoolCreateInfo::flags に vk::CommandPoolCreateFlagBits::eResetCommandBuffer を指定すると、
    // そのコマンドプールから作成したコマンドバッファはリセット可能になる
    cmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    *result = device.get().createCommandPoolUnique(cmdPoolCreateInfo);
    return result;
}

std::shared_ptr<std::vector<vk::UniqueCommandBuffer>> getCommandBuffer(vk::UniqueDevice& device, vk::UniqueCommandPool& cmdPool)
{
    std::shared_ptr<std::vector<vk::UniqueCommandBuffer>> result = std::make_shared<std::vector<vk::UniqueCommandBuffer>>();
    
    // コマンドバッファの作成
    // コマンドバッファを毎フレーム記録しなおせるようにするのは、描画対象のイメージを切り替えるという以外にも意味がある。
    // 今は単なる三角形の表示ですが、動くアニメーションになれば毎フレーム描画内容が変わる
    // そこでフレーム毎にコマンド(描画命令)を変えるのは自然なこと

    // 作るコマンドバッファの数はCommandBufferAllocateInfoの commandBufferCount で指定する
    // commandPoolにはコマンドバッファの作成に使うコマンドプールを指定する
    // このコードではUniqueCommandPoolを使っているので.get()を呼び出して生のCommandPoolを取得している
    vk::CommandBufferAllocateInfo cmdBufAllocInfo;
    cmdBufAllocInfo.commandPool = cmdPool.get();
    cmdBufAllocInfo.commandBufferCount = 1;
    cmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    
    // allocateCommandBufferではなくallocateCommandBuffersである名前から分かる通り、一度にいくつも作れる仕様になっている
    *result = device.get().allocateCommandBuffersUnique(cmdBufAllocInfo);
    return result;
}
