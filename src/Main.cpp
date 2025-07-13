#include <iostream>
#include <memory>
#include <chrono>
#include <functional>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include "../include/Utility.hpp"
#include "../include/Debug.hpp"
#include "../include/Glfw.hpp"
#include "../include/Device.hpp"
#include "../include/PhysicalDevice.hpp"
#include "../include/Surface.hpp"
#include "../include/Swapchain.hpp"
#include "../include/FrameBuffer.hpp"
#include "../include/Pipeline.hpp"
#include "../include/RenderPass.hpp"
#include "../include/Subpass.hpp"
#include "../include/Command.hpp"
#include "../include/Instance.hpp"
#include "../include/ShaderData.hpp"
#include "../include/Texture.hpp"
#include "../include/Depth.hpp"

using namespace Vulkan_Test;

const uint32_t screenWidth = 640;
const uint32_t screenHeight = 480;
const char* windowName = "GLFW Test Window";

int main()
{
    std::shared_ptr<GLFWwindow> window = getGlfwWindow(screenWidth, screenHeight, windowName);
    debugGlfwWindow(*window);

    std::shared_ptr<vk::ApplicationInfo> appInfo = getAppInfo();
    debugApplicationInfo(*appInfo);
    
    std::shared_ptr<vk::UniqueInstance> instance = getInstance(*appInfo);
    std::shared_ptr<vk::UniqueSurfaceKHR> surface = getSurface(*instance, *window);

    std::shared_ptr<std::vector<vk::PhysicalDevice>> physicalDevices = getPhysicalDevices(*instance);
    debugPhysicalDevices(*physicalDevices); 

    std::shared_ptr<std::pair<vk::PhysicalDevice, uint32_t>> physicalDeviceAndQueueFamilyIndex = selectPhysicalDeviceAndQueueFamilyIndex(*physicalDevices, *surface);
    vk::PhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
    std::tie(physicalDevice, queueFamilyIndex) = *physicalDeviceAndQueueFamilyIndex;
    debugPhysicalDevice(physicalDevice, queueFamilyIndex);
    debugPhysicalMemory(physicalDevice);
    
    std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
    debugQueueFamilyProperties(queueProps);
    
    std::shared_ptr<vk::UniqueDevice> device = getDevice(physicalDevice, queueFamilyIndex);
    
    vk::Queue graphicsQueue = device->get().getQueue(queueFamilyIndex, 0);

    std::shared_ptr<std::vector<vk::VertexInputBindingDescription>> vertexBindingDescription = getVertexBindingDescription();
    std::shared_ptr<std::vector<vk::VertexInputAttributeDescription>> vertexInputDescription = getVertexInputDescription();

    std::shared_ptr<vk::UniqueBuffer> vertexBuf = getVertexBuffer(*device);
    std::shared_ptr<vk::UniqueDeviceMemory> vertexBufMem = getVertexBufferMemory(*device, physicalDevice, *vertexBuf);
    std::shared_ptr<vk::UniqueBuffer> stagingVertexBuf = getStagingVertexBuffer(*device, physicalDevice);
    std::shared_ptr<vk::UniqueDeviceMemory> stagingVertexBufMem = getStagingVertexBufferMemory(*device, physicalDevice, *stagingVertexBuf);
    writeStagingVertexBuffer(*device, *stagingVertexBufMem);
    sendVertexBuffer(*device, queueFamilyIndex, graphicsQueue, *stagingVertexBuf, *vertexBuf);

    std::shared_ptr<vk::UniqueBuffer> indexBuf = getIndexBuffer(*device);
    std::shared_ptr<vk::UniqueDeviceMemory> indexBufMem = getIndexBufferMemory(*device, physicalDevice, *indexBuf);
    std::shared_ptr<vk::UniqueBuffer> stagingIndexBuf = getStagingIndexBuffer(*device, physicalDevice);
    std::shared_ptr<vk::UniqueDeviceMemory> stagingIndexBufMem = getStagingIndexBufferMemory(*device, physicalDevice, *stagingIndexBuf);
    writeStagingIndexBuffer(*device, *stagingIndexBufMem);
    sendIndexBuffer(*device, queueFamilyIndex, graphicsQueue, *stagingIndexBuf, *indexBuf);

    int imgWidth, imgHeight, imgCh;
    void* imgData = getImageData(&imgWidth, &imgHeight, &imgCh);
    std::shared_ptr<vk::UniqueImage> texImage = getImage(*device, imgWidth, imgHeight, imgCh);
    std::shared_ptr<vk::UniqueDeviceMemory> imgBufMemory = getImageMemory(*device, physicalDevice, *texImage);
    std::shared_ptr<vk::UniqueBuffer> imgStagingBuf = getStagingImageBuffer(*device, imgWidth, imgHeight, imgCh);
    std::shared_ptr<vk::UniqueDeviceMemory> imgStagingBufMemory = getStagingImageMemory(*device, physicalDevice, *imgStagingBuf);
    writeImageBuffer(*device, *imgStagingBufMemory, imgData, imgWidth, imgHeight, imgCh);
    sendImageBuffer(*device, queueFamilyIndex, graphicsQueue, *imgStagingBuf, *texImage, imgWidth, imgHeight);
    std::shared_ptr<vk::UniqueSampler> texSampler = getSampler(*device);
    std::shared_ptr<vk::UniqueImageView> texImageView = getImageView(*device, *texImage);
    releaseImageData(imgData);

    std::shared_ptr<vk::UniqueBuffer> uniformBuf = getUniformBuffer(*device);
    std::shared_ptr<vk::UniqueDeviceMemory> uniformBufMem = getUniformBufferMemory(*device, physicalDevice, *uniformBuf);
    void* pUniformBufMem = mapUniformBuffer(*device, *uniformBufMem);
    std::shared_ptr<std::vector<vk::UniqueDescriptorSetLayout>> descSetLayouts = getDiscriptorSetLayouts(*device);
    std::shared_ptr<std::vector<vk::DescriptorSetLayout>> unwrapedDescSetLayouts = unwrapHandles<vk::DescriptorSetLayout, vk::UniqueDescriptorSetLayout>(*descSetLayouts);
    std::shared_ptr<vk::UniqueDescriptorPool> descPool = getDescriptorPool(*device);
    std::shared_ptr<std::vector<vk::UniqueDescriptorSet>> descSets = getDescprotorSets(*device, *descPool, *unwrapedDescSetLayouts);
    writeDescriptorSets(*device, *descSets, *uniformBuf, *texImageView, *texSampler);
    std::shared_ptr<std::vector<vk::PushConstantRange>> pushConstantRanges = getPushConstantRanges();

    std::shared_ptr<vk::UniquePipelineLayout> descpriptorPipelineLayout = getDescpriptorPipelineLayout(*device, *unwrapedDescSetLayouts, *pushConstantRanges);
    std::shared_ptr<vk::SurfaceCapabilitiesKHR> surfaceCapabilities = getSurfaceCapabilities(physicalDevice, *surface);
    std::shared_ptr<std::vector<vk::SurfaceFormatKHR>> surfaceFormats = getSurfaceFormats(physicalDevice, *surface);
    std::shared_ptr<std::vector<vk::PresentModeKHR>> surfacePresentModes = getSurfacePresentModes(physicalDevice, *surface);
    vk::SurfaceFormatKHR& surfaceFormat = (*surfaceFormats)[0];
    vk::PresentModeKHR& surfacePresentMode = (*surfacePresentModes)[0];

    std::shared_ptr<std::vector<vk::AttachmentReference>> subpass0_attachmentRefs = getAttachmentReferences();
    std::shared_ptr<vk::AttachmentReference> subpass0_depthStencilAttachmentRef = getDepthStencilAttachmentReference();
    std::shared_ptr<std::vector<vk::SubpassDescription>> subpasses = getSubpassDescription(*subpass0_attachmentRefs, *subpass0_depthStencilAttachmentRef);

    std::shared_ptr<vk::UniqueRenderPass> renderPass = getRenderPass(*device, surfaceFormat, *subpasses);
    std::shared_ptr<vk::UniquePipeline> pipeline = getPipeline(*device, *renderPass, *surfaceCapabilities, *vertexBindingDescription, *vertexInputDescription, *descpriptorPipelineLayout);

    std::shared_ptr<vk::UniqueSwapchainKHR> swapchain;
    std::shared_ptr<std::vector<vk::Image>> swapchainImages;
    std::shared_ptr<std::vector<vk::UniqueImageView>> swapchainImageViews;
    std::shared_ptr<vk::UniqueImage> depthImage;
    std::shared_ptr<vk::UniqueDeviceMemory> depthImageMemory;
    std::shared_ptr<vk::UniqueImageView> depthImageView;
    std::shared_ptr<std::vector<vk::UniqueFramebuffer>> swapchainFramebufs;

    std::function recreateSwapchain = [&]()
    {
        if (swapchainFramebufs)
        {
            swapchainFramebufs->clear();
        }
        if (depthImageView)
        {
            depthImageView->reset();
        }
        if (depthImageMemory)
        {
            depthImageMemory->reset();
        }
        if (depthImage)
        {
            depthImage->reset();
        }
        if (swapchainImageViews)
        {
            swapchainImageViews->clear();
        }
        if (swapchainImages)
        {
            swapchainImages->clear();
        }
        if (swapchain)
        {
            swapchain->reset();
        }

        swapchain = getSwapchain(*device, physicalDevice, *surface, *surfaceCapabilities, surfaceFormat, surfacePresentMode);
        swapchainImages = getSwapchainImages(*device, *swapchain);
        swapchainImageViews = getSwapchainImageViews(*device, *swapchain, *swapchainImages, surfaceFormat);
        depthImage = getDepthImage(*device, physicalDevice, *surfaceCapabilities);
        depthImageMemory = getDepthImageMemory(*device, physicalDevice, *depthImage);
        depthImageView = getDepthImageView(*device, *renderPass, *depthImage);
        swapchainFramebufs = getFramebuffers(*device, *renderPass, *swapchainImageViews, *surfaceCapabilities, *depthImageView);
    };

    recreateSwapchain();

    std::shared_ptr<vk::UniqueCommandPool> cmdPool = getCommandPool(*device, queueFamilyIndex);
    std::shared_ptr<std::vector<vk::UniqueCommandBuffer>> cmdBufs = getCommandBuffer(*device, *cmdPool);

    vk::SemaphoreCreateInfo semaphoreCreateInfo;

    vk::UniqueSemaphore swapchainImgSemaphore = device->get().createSemaphoreUnique(semaphoreCreateInfo);
    vk::UniqueSemaphore imgRenderedSemaphore = device->get().createSemaphoreUnique(semaphoreCreateInfo);

    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    vk::UniqueFence imgRenderedFence = device->get().createFenceUnique(fenceCreateInfo);

    int deltaTime = 0;
    std::chrono::system_clock::time_point sT;

    while (!glfwWindowShouldClose(window.get()))
    {
        sT = std::chrono::system_clock::now();
        
        glfwPollEvents();

        vk::Result waitForFencesResult = device->get().waitForFences({ imgRenderedFence.get() }, VK_TRUE, UINT64_MAX);
        if (waitForFencesResult != vk::Result::eSuccess)
        {
            LOGERR("Failed to get next frame");
            return EXIT_FAILURE;
        }

        vk::ResultValue acquireImgResult = device->get().acquireNextImageKHR(swapchain->get(), UINT64_MAX, swapchainImgSemaphore.get());

        // 再作成処理
        if(acquireImgResult.result == vk::Result::eSuboptimalKHR || acquireImgResult.result == vk::Result::eErrorOutOfDateKHR) 
        {
            LOGERR("Recreate swapchain : " << to_string(acquireImgResult.result));
            recreateSwapchain();
            continue;
        }
        if (acquireImgResult.result != vk::Result::eSuccess)
        {
            LOGERR("Failed to get next frame");
            return EXIT_FAILURE;
        }

        device->get().resetFences({ imgRenderedFence.get() });
        
        writeUniformBuffer(pUniformBufMem, *device, *uniformBufMem, screenWidth, screenHeight, deltaTime);
        
        uint32_t imgIndex = acquireImgResult.value;
    
        (*cmdBufs)[0]->reset();
    
        vk::CommandBufferBeginInfo cmdBeginInfo;
        (*cmdBufs)[0]->begin(cmdBeginInfo);
        
        vk::ClearValue clearVal[2];
        clearVal[0].color.float32[0] = 0.0f;
        clearVal[0].color.float32[1] = 0.0f;
        clearVal[0].color.float32[2] = 0.0f;
        clearVal[0].color.float32[3] = 1.0f;

        // 深度バッファの値は最初は1.0fにクリアされている必要がある
        // 手前かどうかを判定するためのものなので、初期値は何よりも遠くになっていなければならない
        // クリッピングにより1.0より遠くは描画されないので、1.0より大きい値でクリアする必要はない
        clearVal[1].depthStencil.depth = 1.0f;

        vk::RenderPassBeginInfo renderpassBeginInfo;
        renderpassBeginInfo.renderPass = renderPass->get();
        renderpassBeginInfo.framebuffer = (*swapchainFramebufs)[imgIndex].get();
        renderpassBeginInfo.renderArea = vk::Rect2D({ 0,0 }, surfaceCapabilities->currentExtent);
        renderpassBeginInfo.clearValueCount = 2;
        renderpassBeginInfo.pClearValues = clearVal;

        (*cmdBufs)[0]->beginRenderPass(renderpassBeginInfo, vk::SubpassContents::eInline);

        (*cmdBufs)[0]->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->get());
        (*cmdBufs)[0]->bindVertexBuffers(0, { vertexBuf->get() }, { 0 }); 
        (*cmdBufs)[0]->bindIndexBuffer(indexBuf->get(), 0, vk::IndexType::eUint16);
        (*cmdBufs)[0]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, descpriptorPipelineLayout->get(), 0, { (*descSets)[0].get() }, {});

        writePushConstant(0);
        (*cmdBufs)[0]->pushConstants(descpriptorPipelineLayout->get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(ObjectData), &objectData);
        (*cmdBufs)[0]->drawIndexed(indices.size(), 1, 0, 0, 0);
        
        writePushConstant(1);
        (*cmdBufs)[0]->pushConstants(descpriptorPipelineLayout->get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(ObjectData), &objectData);
        (*cmdBufs)[0]->drawIndexed(indices.size(), 1, 0, 0, 0);
    
        (*cmdBufs)[0]->endRenderPass();

        (*cmdBufs)[0]->end();
        
        vk::CommandBuffer submitCmdBuf[1] = { (*cmdBufs)[0].get() };
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = submitCmdBuf;

        // 待機するセマフォの指定
        vk::Semaphore renderwaitSemaphores[] = { swapchainImgSemaphore.get() };
        vk::PipelineStageFlags renderwaitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = renderwaitSemaphores;
        submitInfo.pWaitDstStageMask = renderwaitStages;

        // 完了時にシグナル状態にするセマフォを指定
        vk::Semaphore renderSignalSemaphores[] = { imgRenderedSemaphore.get() };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = renderSignalSemaphores;

        graphicsQueue.submit({ submitInfo }, imgRenderedFence.get());
    
        vk::PresentInfoKHR presentInfo;

        std::vector<vk::SwapchainKHR> presentSwapchains = { swapchain->get() };
        std::vector<uint32_t> imgIndices = { imgIndex };
        
        presentInfo.swapchainCount = presentSwapchains.size();
        presentInfo.pSwapchains = presentSwapchains.data();
        presentInfo.pImageIndices = imgIndices.data();

        // 待機するセマフォの指定
        vk::Semaphore presenWaitSemaphores[] = { imgRenderedSemaphore.get() };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = presenWaitSemaphores;
        
        vk::Result presentResult = graphicsQueue.presentKHR(presentInfo);

        if (presentResult != vk::Result::eSuccess)
        {
            LOGERR("Failed to get next frame");
            return EXIT_FAILURE;
        }

        deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - sT).count();
    }

    unmapUniformBuffer(*device, *uniformBufMem);

    graphicsQueue.waitIdle();
    glfwTerminate();

    return EXIT_SUCCESS;
}