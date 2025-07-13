#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb/stb_image.h"
#include "Utility.hpp"
#include "Debug.hpp"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

void* getImageData(android_app* pApp, int* imgWidth, int* imgHeight, int* imgCh)
{
    std::filesystem::path imagePath = "image.jpg";
    std::string imagePathStr = imagePath.string();

    AAssetManager* assetManager = pApp->activity->assetManager;
    AAsset* asset = AAssetManager_open(assetManager, imagePathStr.c_str(), AASSET_MODE_BUFFER);

    LOG("load image : " << imagePathStr);

    std::vector<unsigned char> fileData;
    if (asset)
    {
        size_t fileSize = AAsset_getLength(asset);
        fileData.resize(fileSize);
        AAsset_read(asset, fileData.data(), fileSize);
        AAsset_close(asset);
    }
    else if (fileData.empty())
    {
        LOG("Failed to load image file." << imagePathStr);
        exit(EXIT_FAILURE);
    }

    int success = stbi_info_from_memory(
            fileData.data(),
            fileData.size(),
            imgWidth,
            imgHeight,
            nullptr);

    if (!success)
    {
        LOG("Failed to load image file." << imagePathStr);
        exit(EXIT_FAILURE);
    }

    int channels;
    stbi_uc* pImgData = stbi_load_from_memory(
            fileData.data(),
            fileData.size(),
            imgWidth,
            imgHeight,
            &channels,
            STBI_rgb_alpha);
    *imgCh = 4;

    if (pImgData == nullptr)
    {
        LOG("Failed to load image file." << imagePathStr);
        exit(EXIT_FAILURE);
    }

    return pImgData;
}

#else

void* getImageData(int* imgWidth, int* imgHeight, int* imgCh)
{
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path imagePath = currentPath / ".." / "assets" / "image.jpg";
    std::string imagePathStr = imagePath.string();

    LOG("load image : " << imagePathStr);
    
    // Vulkanに画像データを渡すには圧縮されていない生データでなければない
    stbi_uc* pImgData = stbi_load(imagePathStr.c_str(), imgWidth, imgHeight, nullptr, STBI_rgb_alpha);
    *imgCh = 4;

    if (pImgData == nullptr) 
    {
       LOG("Failed to load image file." << imagePathStr);
       exit(EXIT_FAILURE);
    }

    return pImgData;
}

#endif

void releaseImageData(void* pImgData)
{
    stbi_image_free(pImgData);
}
