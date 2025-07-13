#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

std::shared_ptr<std::vector<vk::UniqueFramebuffer>> getFramebuffers(vk::UniqueDevice& device, vk::UniqueRenderPass& renderPass, std::vector<vk::UniqueImageView>& swapchainImageViews, vk::SurfaceCapabilitiesKHR& surfaceCapabilities, vk::UniqueImageView& depthImageView)
{
    // レンダーパスは処理(サブパス)とデータ(アタッチメント)のつながりと関係性を記述するが、具体的な処理内容やどのデータを扱うかについては関与しない
    // 具体的な処理内容はコマンドバッファに積むコマンドやパイプラインによって決まるが、具体的なデータの方を決めるためのものがフレームバッファである
    
    // フレームバッファはレンダーパスのアタッチメントとイメージビューを対応付けるもの
    // 今回は画面へ表示するにあたり、スワップチェーンの各イメージビューに描画していく
    // ということは、フレームバッファは1つだけではなくイメージビューごとに作成する必要がある

    std::shared_ptr<std::vector<vk::UniqueFramebuffer>> result = std::make_shared<std::vector<vk::UniqueFramebuffer>>(swapchainImageViews.size());
    
    for (size_t i = 0; i < swapchainImageViews.size(); i++) 
    {
        // イメージビューの情報を初期化用構造体に入れている
        // これで0番のアタッチメントがどのイメージビューに対応しているのかを示すことができる
        vk::ImageView frameBufAttachments[2];
        frameBufAttachments[0] = swapchainImageViews[i].get();
        frameBufAttachments[1] = depthImageView.get();  // 追加

        // フレームバッファを介して「0番のアタッチメントはこのイメージビュー、1番のアタッチメントは…」という結び付けを行うことで初めてレンダーパスが使える
        vk::FramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.width = surfaceCapabilities.currentExtent.width;
        framebufferCreateInfo.height = surfaceCapabilities.currentExtent.height;
        framebufferCreateInfo.layers = 1;
        framebufferCreateInfo.renderPass = renderPass.get();
        framebufferCreateInfo.attachmentCount = std::size(frameBufAttachments);
        framebufferCreateInfo.pAttachments = frameBufAttachments;

        // ここで注意だが、初期化用構造体にレンダーパスの情報を入れてはいるものの、これでレンダーパスとイメージビューが結びついた訳ではない
        // ここで入れているレンダーパスの情報はあくまで「このフレームバッファはどのレンダーパスと結びつけることができるのか」を表しているに過ぎず、
        // フレームバッファを作成した時点で結びついた訳ではない
        // フレームバッファとレンダーパスを本当に結びつける処理はこの次に行う
        // 
        // パイプラインの作成処理でもレンダーパスの情報を渡しているが、ここにも同じ事情がある
        // フレームバッファとパイプラインは特定のレンダーパスに依存して作られるものであり、互換性のない他のレンダーパスのために働こうと思ってもそのようなことはできない
        // 結びつけを行っている訳ではないのにレンダーパスの情報を渡さなければならないのはそのため
        (*result)[i] = device.get().createFramebufferUnique(framebufferCreateInfo);
    }

    return result;
}