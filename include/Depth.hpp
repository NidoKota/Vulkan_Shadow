#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

std::shared_ptr<vk::UniqueImage> getDepthImage(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::SurfaceCapabilitiesKHR& surfaceCapabilities)
{
    std::shared_ptr<vk::UniqueImage> result = std::make_shared<vk::UniqueImage>();
    
    const vk::FormatProperties depthFormatProps = physicalDevice.getFormatProperties(vk::Format::eD32Sfloat);
    
    vk::ImageCreateInfo depthImgCreateInfo;
    depthImgCreateInfo.imageType = vk::ImageType::e2D;
    depthImgCreateInfo.extent = vk::Extent3D(surfaceCapabilities.currentExtent.width, surfaceCapabilities.currentExtent.height, 1);
    depthImgCreateInfo.mipLevels = 1;
    depthImgCreateInfo.arrayLayers = 1;
    depthImgCreateInfo.format = vk::Format::eD32Sfloat;
    depthImgCreateInfo.tiling = vk::ImageTiling::eOptimal;
    depthImgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    depthImgCreateInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    depthImgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    depthImgCreateInfo.samples = vk::SampleCountFlagBits::e1;

    *result = device->createImageUnique(depthImgCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getDepthImageMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueImage& depthImage)
{
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements depthImgMemReq = device->getImageMemoryRequirements(depthImage.get());
    vk::MemoryAllocateInfo depthImgMemAllocInfo;
    depthImgMemAllocInfo.allocationSize = depthImgMemReq.size;

    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (depthImgMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) 
        {
            depthImgMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        LOGERR("Suitable memory type not found.");
        exit(EXIT_FAILURE);
    }
    
    *result = device->allocateMemoryUnique(depthImgMemAllocInfo);
    device->bindImageMemory(depthImage.get(), (*result).get(), 0);
    return result;
}

std::shared_ptr<vk::UniqueImageView> getDepthImageView(vk::UniqueDevice& device, vk::UniqueRenderPass& renderPass, vk::UniqueImage& depthImage)
{
    std::shared_ptr<vk::UniqueImageView> result = std::make_shared<vk::UniqueImageView>();
    
    vk::ImageViewCreateInfo depthImgViewCreateInfo;
    depthImgViewCreateInfo.image = depthImage.get();
    depthImgViewCreateInfo.viewType = vk::ImageViewType::e2D;
    depthImgViewCreateInfo.format = vk::Format::eD32Sfloat;
    depthImgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    depthImgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    depthImgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    depthImgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
    depthImgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    depthImgViewCreateInfo.subresourceRange.baseMipLevel = 0;
    depthImgViewCreateInfo.subresourceRange.levelCount = 1;
    depthImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    depthImgViewCreateInfo.subresourceRange.layerCount = 1;

    *result = device->createImageViewUnique(depthImgViewCreateInfo);
    return result;
}