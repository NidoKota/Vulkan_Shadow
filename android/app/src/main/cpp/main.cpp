#ifdef __ANDROID__
#include <jni.h>
#endif

extern "C"
{
#include <game-activity/native_app_glue/android_native_app_glue.c>
};

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

#include <memory>
#include <functional>
#include "vulkan/vulkan.hpp"
#include "AndroidOut.h"
#include "Utility.hpp"
#include "Debug.hpp"
#include "Device.hpp"
#include "PhysicalDevice.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "FrameBuffer.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Subpass.hpp"
#include "Command.hpp"
#include "Instance.hpp"
#include "ShaderData.hpp"
#include "Texture.hpp"
#include "Depth.hpp"

// グローバル変数や、アプリケーションの状態を管理するクラスのメンバーとして定義
bool g_vulkanInitialized = false;

std::shared_ptr<vk::ApplicationInfo> appInfo;
std::shared_ptr<vk::UniqueInstance> instance;
std::shared_ptr<vk::UniqueSurfaceKHR> surface;
std::shared_ptr<std::vector<vk::PhysicalDevice>> physicalDevices;
std::shared_ptr<std::pair<vk::PhysicalDevice, uint32_t>> physicalDeviceAndQueueFamilyIndex;
vk::PhysicalDevice physicalDevice;
uint32_t queueFamilyIndex;
std::vector<vk::QueueFamilyProperties> queueProps;
std::shared_ptr<vk::UniqueDevice> device;
vk::Queue graphicsQueue;
std::shared_ptr<std::vector<vk::VertexInputBindingDescription>> vertexBindingDescription;
std::shared_ptr<std::vector<vk::VertexInputAttributeDescription>> vertexInputDescription;
std::shared_ptr<vk::UniqueBuffer> vertexBuf;
std::shared_ptr<vk::UniqueDeviceMemory> vertexBufMem;
std::shared_ptr<vk::UniqueBuffer> stagingVertexBuf;
std::shared_ptr<vk::UniqueDeviceMemory> stagingVertexBufMem;
std::shared_ptr<vk::UniqueBuffer> indexBuf;
std::shared_ptr<vk::UniqueDeviceMemory> indexBufMem;
std::shared_ptr<vk::UniqueBuffer> stagingIndexBuf;
std::shared_ptr<vk::UniqueDeviceMemory> stagingIndexBufMem;
int imgWidth, imgHeight, imgCh;
std::shared_ptr<vk::UniqueImage> texImage;
std::shared_ptr<vk::UniqueDeviceMemory> imgBufMemory;
std::shared_ptr<vk::UniqueBuffer> imgStagingBuf;
std::shared_ptr<vk::UniqueDeviceMemory> imgStagingBufMemory;
std::shared_ptr<vk::UniqueSampler> texSampler;
std::shared_ptr<vk::UniqueImageView> texImageView;
std::shared_ptr<vk::UniqueBuffer> uniformBuf;
std::shared_ptr<vk::UniqueDeviceMemory> uniformBufMem;
void* pUniformBufMem;
std::shared_ptr<std::vector<vk::UniqueDescriptorSetLayout>> descSetLayouts;
std::shared_ptr<std::vector<vk::DescriptorSetLayout>> unwrapedDescSetLayouts;
std::shared_ptr<vk::UniqueDescriptorPool> descPool;
std::shared_ptr<std::vector<vk::UniqueDescriptorSet>> descSets;
std::shared_ptr<std::vector<vk::PushConstantRange>> pushConstantRanges;
std::shared_ptr<vk::UniquePipelineLayout> descpriptorPipelineLayout;
std::shared_ptr<vk::SurfaceCapabilitiesKHR> surfaceCapabilities;
std::shared_ptr<std::vector<vk::SurfaceFormatKHR>> surfaceFormats;
std::shared_ptr<std::vector<vk::PresentModeKHR>> surfacePresentModes;
vk::SurfaceFormatKHR surfaceFormat;
vk::PresentModeKHR surfacePresentMode;
std::shared_ptr<std::vector<vk::AttachmentReference>> subpass0_attachmentRefs;
std::shared_ptr<vk::AttachmentReference> subpass0_depthStencilAttachmentRef;
std::shared_ptr<std::vector<vk::SubpassDescription>> subpasses;
std::shared_ptr<vk::UniqueRenderPass> renderPass;
std::shared_ptr<vk::UniquePipeline> pipeline;
std::shared_ptr<vk::UniqueSwapchainKHR> swapchain;
std::shared_ptr<std::vector<vk::Image>> swapchainImages;
std::shared_ptr<std::vector<vk::UniqueImageView>> swapchainImageViews;
std::shared_ptr<vk::UniqueImage> depthImage;
std::shared_ptr<vk::UniqueDeviceMemory> depthImageMemory;
std::shared_ptr<vk::UniqueImageView> depthImageView;
std::shared_ptr<std::vector<vk::UniqueFramebuffer>> swapchainFramebufs;
std::function<void(void)> recreateSwapchain;
std::shared_ptr<vk::UniqueCommandPool> cmdPool;
std::shared_ptr<std::vector<vk::UniqueCommandBuffer>> cmdBufs;
vk::SemaphoreCreateInfo semaphoreCreateInfo;
vk::UniqueSemaphore swapchainImgSemaphore;
vk::UniqueSemaphore imgRenderedSemaphore;
vk::FenceCreateInfo fenceCreateInfo;
vk::UniqueFence imgRenderedFence;
std::chrono::system_clock::time_point sT;
int deltaTime;

// Vulkanを初期化する関数
void initVulkan(android_app* pApp) {
    if (pApp->window == nullptr) {
        LOGERR("initVulkanが呼ばれましたが、ウィンドウがありません。");
        return;
    }
    LOG("ウィンドウが準備されたのでVulkanを初期化します。");
    // ... ここにVulkanの初期化コードを記述 ...

    LOG("vulkan start");

    appInfo = getAppInfo();
    debugApplicationInfo(*appInfo);

    instance = getInstance(*appInfo);
    surface = getSurface(*instance, pApp->window);
    physicalDevices = getPhysicalDevices(*instance);
    debugPhysicalDevices(*physicalDevices);

    physicalDeviceAndQueueFamilyIndex = selectPhysicalDeviceAndQueueFamilyIndex(*physicalDevices, *surface);
    std::tie(physicalDevice, queueFamilyIndex) = *physicalDeviceAndQueueFamilyIndex;
    debugPhysicalDevice(physicalDevice, queueFamilyIndex);
    debugPhysicalMemory(physicalDevice);

    queueProps = physicalDevice.getQueueFamilyProperties();
    debugQueueFamilyProperties(queueProps);

    device = getDevice(physicalDevice, queueFamilyIndex);

    graphicsQueue = device->get().getQueue(queueFamilyIndex, 0);

    vertexBindingDescription = getVertexBindingDescription();
    vertexInputDescription = getVertexInputDescription();

    vertexBuf = getVertexBuffer(*device);
    vertexBufMem = getVertexBufferMemory(*device, physicalDevice, *vertexBuf);
    stagingVertexBuf = getStagingVertexBuffer(*device, physicalDevice);
    stagingVertexBufMem = getStagingVertexBufferMemory(*device, physicalDevice, *stagingVertexBuf);
    writeStagingVertexBuffer(*device, *stagingVertexBufMem);
    sendVertexBuffer(*device, queueFamilyIndex, graphicsQueue, *stagingVertexBuf, *vertexBuf);

    indexBuf = getIndexBuffer(*device);
    indexBufMem = getIndexBufferMemory(*device, physicalDevice, *indexBuf);
    stagingIndexBuf = getStagingIndexBuffer(*device, physicalDevice);
    stagingIndexBufMem = getStagingIndexBufferMemory(*device, physicalDevice, *stagingIndexBuf);
    writeStagingIndexBuffer(*device, *stagingIndexBufMem);
    sendIndexBuffer(*device, queueFamilyIndex, graphicsQueue, *stagingIndexBuf, *indexBuf);

    void *imgData = getImageData(pApp, &imgWidth, &imgHeight, &imgCh);
    texImage = getImage(*device, imgWidth, imgHeight, imgCh);
    imgBufMemory = getImageMemory(*device, physicalDevice, *texImage);
    imgStagingBuf = getStagingImageBuffer(*device, imgWidth, imgHeight, imgCh);
    imgStagingBufMemory = getStagingImageMemory(*device, physicalDevice, *imgStagingBuf);
    writeImageBuffer(*device, *imgStagingBufMemory, imgData, imgWidth, imgHeight, imgCh);
    sendImageBuffer(*device, queueFamilyIndex, graphicsQueue, *imgStagingBuf, *texImage, imgWidth, imgHeight);
    texSampler = getSampler(*device);
    texImageView = getImageView(*device, *texImage);
    releaseImageData(imgData);

    uniformBuf = getUniformBuffer(*device);
    uniformBufMem = getUniformBufferMemory(*device, physicalDevice, *uniformBuf);
    pUniformBufMem = mapUniformBuffer(*device, *uniformBufMem);
    descSetLayouts = getDiscriptorSetLayouts(*device);
    unwrapedDescSetLayouts = unwrapHandles<vk::DescriptorSetLayout, vk::UniqueDescriptorSetLayout>(*descSetLayouts);
    descPool = getDescriptorPool(*device);
    descSets = getDescprotorSets(*device, *descPool, *unwrapedDescSetLayouts);
    writeDescriptorSets(*device, *descSets, *uniformBuf, *texImageView, *texSampler);
    pushConstantRanges = getPushConstantRanges();

    descpriptorPipelineLayout = getDescpriptorPipelineLayout(*device, *unwrapedDescSetLayouts, *pushConstantRanges);
    surfaceCapabilities = getSurfaceCapabilities(physicalDevice, *surface);
    surfaceFormats = getSurfaceFormats(physicalDevice, *surface);
    surfacePresentModes = getSurfacePresentModes(physicalDevice, *surface);
    surfaceFormat = (*surfaceFormats)[0];
    surfacePresentMode = (*surfacePresentModes)[0];

    subpass0_attachmentRefs = getAttachmentReferences();
    subpasses = getSubpassDescription(*subpass0_attachmentRefs, *subpass0_depthStencilAttachmentRef);

    renderPass = getRenderPass(*device, surfaceFormat, *subpasses);
    pipeline = getPipeline(pApp, *device, *renderPass, *surfaceCapabilities, *vertexBindingDescription, *vertexInputDescription, *descpriptorPipelineLayout);

    recreateSwapchain = [&]() {
        if (swapchainFramebufs) {
            swapchainFramebufs->clear();
        }
        if (depthImageView) {
            depthImageView->reset();
        }
        if (depthImageMemory) {
            depthImageMemory->reset();
        }
        if (depthImage) {
            depthImage->reset();
        }
        if (swapchainImageViews) {
            swapchainImageViews->clear();
        }
        if (swapchainImages) {
            swapchainImages->clear();
        }
        if (swapchain) {
            swapchain->reset();
        }

        swapchain = getSwapchain(*device, physicalDevice, *surface, *surfaceCapabilities,surfaceFormat, surfacePresentMode);
        swapchainImages = getSwapchainImages(*device, *swapchain);
        swapchainImageViews = getSwapchainImageViews(*device, *swapchain, *swapchainImages, surfaceFormat);
        depthImage = getDepthImage(*device, physicalDevice, *surfaceCapabilities);
        depthImageMemory = getDepthImageMemory(*device, physicalDevice, *depthImage);
        depthImageView = getDepthImageView(*device, *renderPass, *depthImage);
        swapchainFramebufs = getFramebuffers(*device, *renderPass, *swapchainImageViews, *surfaceCapabilities, *depthImageView);
    };

    recreateSwapchain();

    cmdPool = getCommandPool(*device, queueFamilyIndex);
    cmdBufs = getCommandBuffer(*device, *cmdPool);

    swapchainImgSemaphore = device->get().createSemaphoreUnique(semaphoreCreateInfo);
    imgRenderedSemaphore = device->get().createSemaphoreUnique(semaphoreCreateInfo);

    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    imgRenderedFence = device->get().createFenceUnique(fenceCreateInfo);

    deltaTime = 0;
    sT = std::chrono::system_clock::time_point();

    g_vulkanInitialized = true; // 初期化完了フラグを立てる
}

// Vulkanを終了する関数
void terminateVulkan() {
    if (!g_vulkanInitialized) return;

    // ... デバイスの待機やリソースの解放処理 ...

    g_vulkanInitialized = false;

    unmapUniformBuffer(*device, *uniformBufMem);

    graphicsQueue.waitIdle();

    LOG("Vulkanを終了しました。");
}

// 1フレームを描画する関数
void drawFrame() {
    if (!g_vulkanInitialized) return;
    // ... acquireNextImageKHRからpresentKHRまで、描画ループの1回分の処理 ...

    sT = std::chrono::system_clock::now();

    vk::Result waitForFencesResult = device->get().waitForFences({ imgRenderedFence.get() }, VK_TRUE, UINT64_MAX);
    if (waitForFencesResult != vk::Result::eSuccess)
    {
        LOGERR("Failed to get next frame");
        exit(EXIT_FAILURE);
    }

    vk::ResultValue acquireImgResult = device->get().acquireNextImageKHR(swapchain->get(), UINT64_MAX, swapchainImgSemaphore.get());

    // 再作成処理
    if(acquireImgResult.result == vk::Result::eSuboptimalKHR || acquireImgResult.result == vk::Result::eErrorOutOfDateKHR)
    {
        LOGERR("Recreate swapchain : " << to_string(acquireImgResult.result));
        recreateSwapchain();
        return;
    }
    if (acquireImgResult.result != vk::Result::eSuccess)
    {
        LOGERR("Failed to get next frame");
        exit(EXIT_FAILURE);
    }

    device->get().resetFences({ imgRenderedFence.get() });

    writeUniformBuffer(pUniformBufMem, *device, *uniformBufMem, 1080, 2400, deltaTime);

    uint32_t imgIndex = acquireImgResult.value;

    (*cmdBufs)[0]->reset();

    vk::CommandBufferBeginInfo cmdBeginInfo;
    (*cmdBufs)[0]->begin(cmdBeginInfo);

    vk::ClearValue clearVal[2];
    clearVal[0].color.float32[0] = 0.3f;
    clearVal[0].color.float32[1] = 0.3f;
    clearVal[0].color.float32[2] = 0.3f;
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
        exit(EXIT_FAILURE);
    }

    deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - sT).count();
}


// Androidシステムからのコマンドを処理するコールバック関数
void handle_cmd(android_app *pApp, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            // ウィンドウの準備ができた、という通知。
            // Vulkanの初期化を開始する絶好のタイミング。
            initVulkan(pApp);
            break;

        case APP_CMD_TERM_WINDOW:
            // ウィンドウが破棄される。グラフィックスリソースを解放する。
            terminateVulkan();
            break;

        case APP_CMD_GAINED_FOCUS:
            // アプリがフォーカスを得た（描画再開）
            break;

        case APP_CMD_LOST_FOCUS:
            // アプリがフォーカスを失った（描画停止）
            // drawFrameを一時停止するフラグを立てるなど
            break;
    }
}

int32_t handle_input(struct android_app* app)
{
    auto inputBuffer = android_app_swap_input_buffers(app);
    if ( inputBuffer == nullptr )
    {
        return 0;
    }

    int32_t processed = 0;
    for (int i = 0; i < inputBuffer->motionEventsCount; ++i)
    {
        auto& event = inputBuffer->motionEvents[i];
        auto action = event.action;
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        auto& pointer = event.pointers[pointerIndex];
        auto x= GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);
        auto scrollH = GameActivityPointerAxes_getAxisValue(&pointer, AMOTION_EVENT_AXIS_HSCROLL);
        auto scrollV = GameActivityPointerAxes_getAxisValue(&pointer, AMOTION_EVENT_AXIS_VSCROLL);

        switch(action & AMOTION_EVENT_ACTION_MASK)
        {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_UP:
                // io.AddMousePosEvent(x, y);
                // io.AddMouseButtonEvent(0, action == AMOTION_EVENT_ACTION_DOWN);
                processed = 1;
                break;

            case AMOTION_EVENT_ACTION_HOVER_MOVE:
            case AMOTION_EVENT_ACTION_MOVE:
                // io.AddMousePosEvent(x, y);
                processed = 1;
                break;

            default:
                break;
        }
    }
    android_app_clear_motion_events(inputBuffer);
    return processed;
}

bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

// アプリケーションのエントリーポイント
extern "C" void android_main(struct android_app *pApp) {
    // コールバック関数を登録
    pApp->onAppCmd = handle_cmd;
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);

    // メインループ
    while (true) {
        int events;
        android_poll_source *pSource;

        // イベントを待機・処理する。
        // タイムアウトを-1にすると、イベントが来るまで待機する。
        if (ALooper_pollOnce(0, nullptr, &events, (void **) &pSource) >= 0) {
            if (pSource) {
                pSource->process(pApp, pSource);
            }
        }

        // Androidシステムから終了リクエストがあれば、ループを抜ける
        if (pApp->destroyRequested != 0) {
            // terminateVulkan() は TERM_WINDOW で呼ばれるが、念のため
            return;
        }

        // Vulkanが初期化済みで、アプリがフォーカスを持っているなら描画
        if (g_vulkanInitialized) {
            // lost_focusなどで描画を止めるフラグ制御を入れるのが望ましい
            drawFrame();
        }
    }
}
