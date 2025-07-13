#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

std::shared_ptr<vk::SwapchainCreateInfoKHR> getSwapchainCreateInfo(
    vk::PhysicalDevice& physicalDevice, vk::UniqueSurfaceKHR& surface, 
    vk::SurfaceCapabilitiesKHR& surfaceCapabilities, vk::SurfaceFormatKHR& surfaceFormat, vk::PresentModeKHR& surfacePresentMode)
{
    std::shared_ptr<vk::SwapchainCreateInfoKHR> result = std::make_shared<vk::SwapchainCreateInfoKHR>();

    result->surface = surface.get();
    // minImageCountはスワップチェーンが扱うイメージの数
    // getSurfaceCapabilitiesKHRで得られたminImageCount(最小値)とmaxImageCount(最大値)の間の数なら何枚でも問題ない
    // https://vulkan-tutorial.com/
    // の記述に従って最小値+1を指定する
    result->minImageCount = surfaceCapabilities.minImageCount + 1;
    // imageFormatやimageColorSpaceなどには、スワップチェーンが取り扱う画像の形式などを指定する
    // しかしこれらに指定できる値はサーフェスとデバイスの事情によって制限されるものであり、自由に決めることができるものではない
    // ここに指定する値は、必ずgetSurfaceFormatsKHRが返した配列に含まれる組み合わせでなければならない
    result->imageFormat = surfaceFormat.format;
    result->imageColorSpace = surfaceFormat.colorSpace;
    // imageExtentはスワップチェーンの画像サイズを表す
    // getSurfaceCapabilitiesKHRで得られたminImageExtent(最小値)とmaxImageExtent(最大値)の間でなければならない
    // currentExtentで現在のサイズが得られるため、それを指定
    result->imageExtent = surfaceCapabilities.currentExtent;
    result->imageArrayLayers = 1;
    result->imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    result->imageSharingMode = vk::SharingMode::eExclusive;
    // preTransformは表示時の画面反転・画面回転などのオプションを指定する
    // これもgetSurfaceCapabilitiesKHRの戻り値に依存する
    // supportedTransformsでサポートされている中に含まれるものでなければならないので、無難なcurrentTransform(現在の画面設定)を指定
    result->preTransform = surfaceCapabilities.currentTransform;
    // presentModeは表示処理のモードを示すもの
    // getSurfacePresentModesKHRの戻り値の配列に含まれる値である必要がある
    result->presentMode = surfacePresentMode;
    result->clipped = VK_TRUE;
    return result;
}

std::shared_ptr<vk::UniqueSwapchainKHR> getSwapchain(vk::UniqueDevice& device, vk::SwapchainCreateInfoKHR& swapchainCreateInfo)
{
    std::shared_ptr<vk::UniqueSwapchainKHR> result = std::make_shared<vk::UniqueSwapchainKHR>();
    *result = device.get().createSwapchainKHRUnique(swapchainCreateInfo);
    return result;
}

// 一般的なコンピュータは、アニメーションを描画・表示する際に「描いている途中」が見えないようにするため、
// 2枚以上のキャンバスを用意して、1枚を使って現在のフレームを表示させている裏で別の1枚に次のフレームを描画する、という仕組みを採用している
// スワップチェーンは、一言で言えば「画面に表示されようとしている画像の連なり」
//
// スワップチェーンは自分でイメージを保持している
// 我々はイメージを作成する必要はなく、スワップチェーンが元から保持しているイメージを取得してそこに描画することになる
//
// スワップチェーンからイメージを取得した後は基本的に同じ
// イメージからイメージビューを作成、
// レンダーパス・パイプラインなどの作成、
// フレームバッファを作成してイメージビューと紐づけ、
// コマンドバッファにコマンドを積んでキューに送信
std::shared_ptr<vk::UniqueSwapchainKHR> getSwapchain(
    vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueSurfaceKHR& surface, 
    vk::SurfaceCapabilitiesKHR& surfaceCapabilities, vk::SurfaceFormatKHR& surfaceFormat, vk::PresentModeKHR& surfacePresentMode)
{
    std::shared_ptr<vk::SwapchainCreateInfoKHR> swapchainCreateInfo = getSwapchainCreateInfo(physicalDevice, surface, surfaceCapabilities, surfaceFormat, surfacePresentMode);
    debugSwapchainCreateInfo(*swapchainCreateInfo);
    return getSwapchain(device, *swapchainCreateInfo);
}

std::shared_ptr<std::vector<vk::Image>> getSwapchainImages(vk::UniqueDevice& device, vk::UniqueSwapchainKHR& swapchain)
{
    std::shared_ptr<std::vector<vk::Image>> result = std::make_shared<std::vector<vk::Image>>();
    *result = device.get().getSwapchainImagesKHR(swapchain.get());
    return result;
}

std::shared_ptr<std::vector<vk::UniqueImageView>> getSwapchainImageViews(
    vk::UniqueDevice& device, vk::UniqueSwapchainKHR& swapchain, std::vector<vk::Image>& swapchainImages, vk::SurfaceFormatKHR& surfaceFormat)
{
    std::shared_ptr<std::vector<vk::UniqueImageView>> result = std::make_shared<std::vector<vk::UniqueImageView>>(swapchainImages.size());
    
    for (size_t i = 0; i < result->size(); i++) 
    {
        vk::ImageViewCreateInfo imgViewCreateInfo;
        imgViewCreateInfo.image = swapchainImages[i];
        imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
        // ここに指定するフォーマットは元となるイメージに合わせる必要がある
        // スワップチェーンのイメージの場合、イメージのフォーマットはスワップチェーンを作成する時に指定したフォーマットになるため、その値を指定
        imgViewCreateInfo.format = surfaceFormat.format;
        imgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imgViewCreateInfo.subresourceRange.levelCount = 1;
        imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imgViewCreateInfo.subresourceRange.layerCount = 1;

        (*result)[i] = device.get().createImageViewUnique(imgViewCreateInfo);
    }

    return result;
}