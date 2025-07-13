#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <fstream>
#include <filesystem>
#include "Utility.hpp"
#include "Debug.hpp"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#endif

using namespace Vulkan_Test;

std::shared_ptr<vk::UniquePipelineLayout> getDescpriptorPipelineLayout(vk::UniqueDevice& device, std::vector<vk::DescriptorSetLayout>& descSetLayouts, std::vector<vk::PushConstantRange>& pushConstantRanges)
{
    std::shared_ptr<vk::UniquePipelineLayout> result = std::make_shared<vk::UniquePipelineLayout>();

    vk::PipelineLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.setLayoutCount = descSetLayouts.size();
    layoutCreateInfo.pSetLayouts = descSetLayouts.data();
    layoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
    layoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

    *result = device->createPipelineLayoutUnique(layoutCreateInfo);
    return result;
}

#if defined(__ANDROID__)
std::shared_ptr<vk::UniquePipeline> getPipeline(
        android_app* pApp,
        vk::UniqueDevice& device,
        vk::UniqueRenderPass& renderpass,
        vk::SurfaceCapabilitiesKHR& surfaceCapabilities,
        std::vector<vk::VertexInputBindingDescription>& vertexBindingDescription,
        std::vector<vk::VertexInputAttributeDescription>& vertexInputDescription,
        vk::UniquePipelineLayout& pipelineLayout)
#else
std::shared_ptr<vk::UniquePipeline> getPipeline(
        vk::UniqueDevice& device,
        vk::UniqueRenderPass& renderpass,
        vk::SurfaceCapabilitiesKHR& surfaceCapabilities,
        std::vector<vk::VertexInputBindingDescription>& vertexBindingDescription,
        std::vector<vk::VertexInputAttributeDescription>& vertexInputDescription,
        vk::UniquePipelineLayout& pipelineLayout)
#endif
{
    std::shared_ptr<vk::UniquePipeline> result = std::make_shared<vk::UniquePipeline>();

    // 2種類の頂点入力デスクリプションを作成したら、それをパイプラインに設定する
    // 頂点入力デスクリプションはvk::PipelineVertexInputStateCreateInfo構造体に設定する
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = vertexBindingDescription.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescription.data();
    vertexInputInfo.vertexAttributeDescriptionCount = vertexInputDescription.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputDescription.data();

    // 深度バッファを有効化するための設定を入れる構造体
    vk::PipelineDepthStencilStateCreateInfo depthstencil;
    // depthTestEnableをVK_TRUEにすると、深度バッファの値とZ値の比較による描画スキップ(デプステスト)が有効化される
    depthstencil.depthTestEnable = true;
    // depthWriteEnableをVK_TRUEにすると、ポリゴンを描画した際にそのZ値が深度バッファに書き込まれる
    depthstencil.depthWriteEnable = true;
    // depthCompareOpは、デプステストの際の比較方法を指定
    // ここではeLessを指定しているが、例えばeGreaterなどを指定すると逆の判定になる
    depthstencil.depthCompareOp = vk::CompareOp::eLess;
    depthstencil.depthBoundsTestEnable = false;
    depthstencil.stencilTestEnable = false;
    
    // パイプラインとは、3DCGの基本的な描画処理をひとつながりにまとめたもの
    // パイプラインは「点の集まりで出来た図形を色のついたピクセルの集合に変換するもの」
    // ほぼ全ての3DCGは三角形の集まりであり、私たちが最初に持っているものは三角形の各点の色や座標であるが、
    // 最終的に欲しいものは画面のどのピクセルがどんな色なのかという情報である
    // この間を繋ぐ演算処理は大体お決まりのパターンになっており、まとめてグラフィックスパイプラインとなっている

    // この処理は全ての部分が固定されているものではなく、プログラマ側で色々指定する部分があり、
    // それらの情報をまとめたものがパイプラインオブジェクト(vk::Pipeline)である
    // 実際に使用して描画処理を行う際はコマンドでパイプラインをバインドし、ドローコールを呼ぶ

    // Vulkanにおけるパイプラインには「グラフィックスパイプライン」と「コンピュートパイプライン」の2種類がある
    // コンピュートパイプラインはGPGPUなどに使うもの
    // 今回は普通に描画が目的なのでグラフィックスパイプラインを作成する
    // グラフィックスパイプラインはvk::DeviceのcreateGraphicsPipelineメソッドで作成できる
    
    vk::Viewport viewports[1];
    viewports[0].x = 0.0;
    viewports[0].y = 0.0;
    viewports[0].minDepth = 0.0;
    viewports[0].maxDepth = 1.0;
    viewports[0].width = surfaceCapabilities.currentExtent.width;
    viewports[0].height = surfaceCapabilities.currentExtent.height;

    vk::Rect2D scissors[1];
    scissors[0].offset = vk::Offset2D(0, 0);
    scissors[0].extent = surfaceCapabilities.currentExtent;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = viewports;
    viewportState.scissorCount = 1;
    viewportState.pScissors = scissors;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = false;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = false;
    rasterizer.rasterizerDiscardEnable = false;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = false;

    vk::PipelineMultisampleStateCreateInfo multisample;
    multisample.sampleShadingEnable = false;
    multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState blendattachment[1];
    blendattachment[0].colorWriteMask =
        vk::ColorComponentFlagBits::eA |
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB;
    blendattachment[0].blendEnable = false;

    vk::PipelineColorBlendStateCreateInfo blend;
    blend.logicOpEnable = false;
    blend.attachmentCount = 1;
    blend.pAttachments = blendattachment;

    vk::UniqueShaderModule vertShader;
    vk::UniqueShaderModule fragShader;
    {
#if __ANDROID__
        AAssetManager* assetManager = pApp->activity->assetManager;
#endif

        // 頂点シェーダーを読み込む
#if __ANDROID__
        std::vector<char> vertSpvFileData;
        AAsset* vertSpvFile = AAssetManager_open(assetManager, "shader.vert.spv", AASSET_MODE_BUFFER);
        size_t vertSpvFileSz = AAsset_getLength(vertSpvFile);
        vertSpvFileData.resize(vertSpvFileSz);
        AAsset_read(vertSpvFile, vertSpvFileData.data(), vertSpvFileSz);
        AAsset_close(vertSpvFile);
#else
        size_t vertSpvFileSz = std::filesystem::file_size("../src/shader.vert.spv");
        std::ifstream vertSpvFile = std::ifstream("../src/shader.vert.spv", std::ios_base::binary);
        std::vector<char> vertSpvFileData = std::vector<char>(vertSpvFileSz);
        vertSpvFile.read(vertSpvFileData.data(), vertSpvFileSz);
#endif

        vk::ShaderModuleCreateInfo vertShaderCreateInfo;
        vertShaderCreateInfo.codeSize = vertSpvFileSz;
        vertShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertSpvFileData.data());
        vertShader = device.get().createShaderModuleUnique(vertShaderCreateInfo);

        // フラグメントシェーダーを読み込む
#if __ANDROID__
        std::vector<char> fragSpvFileData;
        AAsset* fragSpvFile = AAssetManager_open(assetManager, "shader.frag.spv", AASSET_MODE_BUFFER);
        size_t fragSpvFileSz = AAsset_getLength(fragSpvFile);
        fragSpvFileData.resize(fragSpvFileSz);
        AAsset_read(fragSpvFile, fragSpvFileData.data(), fragSpvFileSz);
        AAsset_close(fragSpvFile);
#else
        size_t fragSpvFileSz = std::filesystem::file_size("../src/shader.frag.spv");
        std::ifstream fragSpvFile = std::ifstream("../src/shader.frag.spv", std::ios_base::binary);
        std::vector<char> fragSpvFileData = std::vector<char>(fragSpvFileSz);
        fragSpvFile.read(fragSpvFileData.data(), fragSpvFileSz);
#endif

        vk::ShaderModuleCreateInfo fragShaderCreateInfo;
        fragShaderCreateInfo.codeSize = fragSpvFileSz;
        fragShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragSpvFileData.data());
        fragShader = device.get().createShaderModuleUnique(fragShaderCreateInfo);
    }

    vk::PipelineShaderStageCreateInfo shaderStage[2];
    shaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
    shaderStage[0].module = vertShader.get();
    shaderStage[0].pName = "main";
    shaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
    shaderStage[1].module = fragShader.get();
    shaderStage[1].pName = "main";
    
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pRasterizationState = &rasterizer;
    pipelineCreateInfo.pMultisampleState = &multisample;
    pipelineCreateInfo.pColorBlendState = &blend;
    pipelineCreateInfo.pDepthStencilState = &depthstencil;
    pipelineCreateInfo.layout = pipelineLayout.get();
    pipelineCreateInfo.renderPass = renderpass.get();
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.stageCount = std::size(shaderStage);
    pipelineCreateInfo.pStages = shaderStage;

    *result = device.get().createGraphicsPipelineUnique(nullptr, pipelineCreateInfo).value;
    return result;
}