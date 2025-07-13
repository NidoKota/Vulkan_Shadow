#define VK_ENABLE_BETA_EXTENSIONS
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <fstream>
#include <filesystem>
#include "../include/Utility.hpp"
// #include "../include/stb/stb_image_write.h"

using namespace Vulkan_Test;

// https://chaosplant.tech/do/vulkan/
// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

// win
// cd win
// ."C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" .
// VisualStudioでビルド

// mac
// cd mac
// cmake .
// make

// glslc ../src/shader.vert -o ../src/shader.vert.spv
// glslc ../src/shader.frag -o ../src/shader.frag.spv

int main()
{
    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "YourApp";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = vk::ApiVersion11; // Vulkan1.1以降に導入された関数を使うために変更

    if (!glfwInit())
    {
        return EXIT_FAILURE;
    }

    // これに色々情報を乗っけることで
    // 例えばVulkanの拡張機能をオンにしたりだとかデバッグ情報を表示させるといったことができる
    // これはオプション的なもの
    vk::InstanceCreateInfo instCreateInfo;
    instCreateInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionsCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    std::vector<const char*> insRequiredExtensions = std::vector<const char*>(glfwExtensionsCount);
    std::memcpy(&*insRequiredExtensions.begin(), glfwExtensions, sizeof(const char*) * glfwExtensionsCount);

#ifdef __APPLE__
    std::vector<const char*> appleRrequiredExtentions = 
    { 
        vk::KHRPortabilityEnumerationExtensionName,
    };
    std::copy(appleRrequiredExtentions.begin(), appleRrequiredExtentions.end(), std::back_inserter(insRequiredExtensions));
#endif

    instCreateInfo.enabledExtensionCount = insRequiredExtensions.size();
    instCreateInfo.ppEnabledExtensionNames = &*insRequiredExtensions.begin();

    LOG("Instance Extensions");
    for (int i = 0; i < instCreateInfo.enabledExtensionCount; i++) 
    {
        LOG("----------------------------------------");
        LOG(instCreateInfo.ppEnabledExtensionNames[i]);
    }

#ifdef __APPLE__
    instCreateInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    // Vulkanインスタンスを作ってから壊すまでの間に色々な処理を書いていく
    // Vulkanを使う場合、全ての機能はこのインスタンスを介して利用する
    vk::UniqueInstance instancePtr = vk::createInstanceUnique(instCreateInfo);
    vk::Instance instance = instancePtr.get();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    const uint32_t screenWidth = 640;
    const uint32_t screenHeight = 480;

    GLFWwindow* window;
    window = glfwCreateWindow(screenWidth, screenHeight, "GLFW Test Window", NULL, NULL);
    if (!window) 
    {
        const char* err;
        glfwGetError(&err);
        LOG(err);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    int width;
    int height;
    glfwGetWindowSize(window, &width, &height);

    LOG("width " << width << ", height " << height);

    glfwGetFramebufferSize(window, &width, &height);
    LOG("width " << width << ", height " << height);

    // 表示する先の画面は「サーフェス」というオブジェクトで抽象化されている
    // ウィンドウやスクリーンなど、「表示する先として使える何か」は全てサーフェスという同じ種類のオブジェクトで表され、統一的に扱うことができる
    VkSurfaceKHR c_surface;
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &c_surface);
    if (result != VK_SUCCESS) 
    {
        const char* err;
        glfwGetError(&err);
        LOG(err);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    vk::UniqueSurfaceKHR surfacePtr = vk::UniqueSurfaceKHR(c_surface, instance);
    vk::SurfaceKHR surface = surfacePtr.get();

    // GPUが複数あるなら頼む相手をまず選ぶ必要がある
    // GPUの型式などによってサポートしている機能とサポートしていない機能があったりするため、
    // だいたい「インスタンスを介して物理デバイスを列挙する」→「それぞれの物理デバイスの情報を取得する」→「一番いいのを頼む」という流れになる
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    for (vk::PhysicalDevice physicalDevice : physicalDevices)
    {
        vk::PhysicalDeviceProperties2 props = physicalDevice.getProperties2();
        LOG("----------------------------------------");
        LOG(props.properties.deviceName);
        LOG("deviceID: " << props.properties.deviceID);
        LOG("vendorID: " << props.properties.vendorID);
        LOG("deviceType: " << to_string(props.properties.deviceType));
    }

    // デバイスとキューの検索   
    uint32_t graphicsQueueFamilyIndex;
    vk::PhysicalDevice physicalDevice;
    bool existsSuitablePhysicalDevice = false;
    for (size_t i = 0; i < physicalDevices.size(); i++)
    {
        std::vector<vk::QueueFamilyProperties> queueProps = physicalDevices[i].getQueueFamilyProperties();

        bool foundGraphicsQueue = false;
        for (size_t j = 0; j < queueProps.size(); j++)
        {
            // グラフィックス機能に加えてサーフェスへのプレゼンテーションもサポートしているキューを厳選
            if (!(queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics) || !physicalDevices[i].getSurfaceSupportKHR(j, surface)) continue;
            
            foundGraphicsQueue = true;
            graphicsQueueFamilyIndex = j;
            physicalDevice = physicalDevices[i];
            break;
        }

        std::vector<vk::ExtensionProperties> extProps = physicalDevices[i].enumerateDeviceExtensionProperties();
        bool supportsSwapchainExtension = false;

        for (size_t j = 0; j < extProps.size(); j++) 
        {
            // enumerateDeviceExtensionPropertiesメソッドでその物理デバイスがサポートしている拡張機能の一覧を取得
            // その中にスワップチェーンの拡張機能の名前が含まれていなければそのデバイスは使わない
            if (std::string_view(extProps[j].extensionName.data()) == VK_KHR_SWAPCHAIN_EXTENSION_NAME) 
            {
                supportsSwapchainExtension = true;
                break;
            }
        }

        // デバイスがサーフェスを間違いなくサポートしていることを確かめる
        bool supportsSurface =
            !physicalDevices[i].getSurfaceFormatsKHR(surface).empty() ||
            !physicalDevices[i].getSurfacePresentModesKHR(surface).empty();

        if (foundGraphicsQueue && supportsSwapchainExtension && supportsSurface) 
        {
            physicalDevice = physicalDevices[i];
            existsSuitablePhysicalDevice = true;
            break;
        }
    }

    if (existsSuitablePhysicalDevice)
    {
        vk::PhysicalDeviceProperties2 props = physicalDevice.getProperties2();
        LOG("----------------------------------------");
        LOG("Found device and queue");
        LOG("physicalDevice: " << props.properties.deviceName);
        LOG("graphicsQueueFamilyIndex: " << graphicsQueueFamilyIndex);
    }
    else
    {
        LOGERR("No physical devices are available");
        return EXIT_FAILURE;
    }

    // Vulkanにおけるキューとは、GPUの実行するコマンドを保持する待ち行列 = GPUのやることリスト
    // GPUにコマンドを送るときは、このキューにコマンドを詰め込むことになる
    // 
    // 1つのGPUが持っているキューは1つだけとは限らない
    // キューによってサポートしている機能とサポートしていない機能がある
    // キューにコマンドを送るときは、そのキューが何の機能をサポートしているかを事前に把握しておく必要がある
    // 
    // ある物理デバイスの持っているキューの情報は getQueueFamilyProperties メソッドで取得できる
    // メソッド名にある「キューファミリ」というのは、同じ能力を持っているキューをひとまとめにしたもの
    // 1つの物理デバイスには1個以上のキューファミリがあり、1つのキューファミリには1個以上の同等の機能を持ったキューが所属している
    // 1つのキューファミリの情報は vk::QueueFamilyProperties 構造体で表される
    std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
    LOG("----------------------------------------");
    LOG("queue family count: " << queueProps.size());
    for (size_t i = 0; i < queueProps.size(); i++)
    {
        LOG("----------------------------------------");
        LOG("queue family index: " << i);
        LOG("queue count: " << queueProps[i].queueCount);
        LOG(to_string(queueProps[i].queueFlags));
    }

    // Vulkanでは論理デバイスからキューを取得するとき、論理デバイスにあらかじめ「このキューを使うよ」ということを伝えておく必要がある
    // 論理デバイスの作成時、vk::DeviceCreateInfo構造体に「この論理デバイスを通じてこのキューファミリからいくつのキューを使う」という情報を指定する必要がある
    // その情報は vk::DeviceQueueCreateInfo 構造体の配列で表される
    // 
    // 今のところ欲しいキューは1つだけなので要素数1の配列にする
    // queueFamilyIndex はキューの欲しいキューファミリのインデックスを表し、 
    // queueCount はそのキューファミリからいくつのキューが欲しいかを表す
    // 今欲しいのはグラフィック機能をサポートするキューを1つだけ
    // pQueuePriorities はキューのタスク実行の優先度を表すfloat値配列を指定
    // 優先度の値はキューごとに決められるため、この配列はそのキューファミリから欲しいキューの数だけの要素数を持ち
    float queuePriorities[1] = { 1.0f };
    vk::DeviceQueueCreateInfo queueCreateInfo[1];
    queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo[0].pQueuePriorities = queuePriorities;
    queueCreateInfo[0].queueCount = std::size(queueCreateInfo);

    // GPUが複数あるなら頼む相手をまず選ぶ
    // ここで言う「選ぶ」とは、特定のGPUを完全に占有してしまうとかそういう話ではない
    // 
    // 物理デバイスを選んだら次は論理デバイスを作成する
    // VulkanにおいてGPUの能力を使うような機能は、全てこの論理デバイスを通して利用する
    // GPUの能力を使う際、物理デバイスを直接いじることは出来ない
    // 
    // コンピュータの仕組みについて多少知識のある人であれば、
    // この「物理」「論理」の区別はメモリアドレスの「物理アドレス」「論理アドレス」と同じ話であることが想像できる
    // マルチタスクOS上で、1つのプロセスが特定のGPU(=物理デバイス)を独占して管理するような状態はよろしくない
    // 
    // 仮想化されたデバイスが論理デバイス
    // これならあるプロセスが他のプロセスの存在を意識することなくGPUの能力を使うことができる
    
    // インスタンスを作成するときにvk::InstanceCreateInfo構造体を使ったのと同じように、
    // 論理デバイス作成時にもvk::DeviceCreateInfo構造体の中に色々な情報を含めることができる
    std::vector<const char*> requiredLayers = 
    { 
        "VK_LAYER_KHRONOS_validation" 
    };
    vk::DeviceCreateInfo devCreateInfo;
    devCreateInfo.enabledLayerCount = requiredLayers.size();
    devCreateInfo.ppEnabledLayerNames = &*requiredLayers.begin();
    devCreateInfo.queueCreateInfoCount = std::size(queueCreateInfo);
    devCreateInfo.pQueueCreateInfos = queueCreateInfo;

    // スワップチェーンは拡張機能なので、機能を有効化する必要がある
    // スワップチェーンは「デバイスレベル」の拡張機能
    std::vector<const char*> devRequiredExtensions = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME 
    };
    devCreateInfo.enabledExtensionCount = devRequiredExtensions.size();
    devCreateInfo.ppEnabledExtensionNames = &*devRequiredExtensions.begin();

    // vk::Instanceにはそれに対応するvk::UniqueInstanceが存在しましたが、
    // vk::PhysicalDeviceに対応するvk::UniquePhysicalDevice は存在しない
    // destroyなどを呼ぶ必要もない
    // vk::PhysicalDeviceは単に物理的なデバイスの情報を表しているに過ぎないので、構築したり破棄したりする必要がある類のオブジェクトではない
    vk::UniqueDevice devicePtr = physicalDevice.createDeviceUnique(devCreateInfo);
    vk::Device device = devicePtr.get();

    vk::Queue graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);

    // 一般的なコンピュータは、アニメーションを描画・表示する際に「描いている途中」が見えないようにするため、
    // 2枚以上のキャンバスを用意して、1枚を使って現在のフレームを表示させている裏で別の1枚に次のフレームを描画する、という仕組みを採用している
    // スワップチェーンは、一言で言えば「画面に表示されようとしている画像の連なり」
    // この3つのAPIは、「サーフェスの情報」および「物理デバイスが対象のサーフェスを扱う能力」の情報を取得するAPIである
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
    std::vector<vk::PresentModeKHR> surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    vk::SurfaceFormatKHR swapchainFormat = surfaceFormats[0];
    vk::PresentModeKHR swapchainPresentMode = surfacePresentModes[0];

    vk::SwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.surface = surface;
    // minImageCountはスワップチェーンが扱うイメージの数
    // getSurfaceCapabilitiesKHRで得られたminImageCount(最小値)とmaxImageCount(最大値)の間の数なら何枚でも問題ない
    // https://vulkan-tutorial.com/
    // の記述に従って最小値+1を指定する
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
    LOG("swapchain minImageCount" << swapchainCreateInfo.minImageCount);
    // imageFormatやimageColorSpaceなどには、スワップチェーンが取り扱う画像の形式などを指定する
    // しかしこれらに指定できる値はサーフェスとデバイスの事情によって制限されるものであり、自由に決めることができるものではない
    // ここに指定する値は、必ずgetSurfaceFormatsKHRが返した配列に含まれる組み合わせでなければならない
    swapchainCreateInfo.imageFormat = swapchainFormat.format;
    swapchainCreateInfo.imageColorSpace = swapchainFormat.colorSpace;
    // imageExtentはスワップチェーンの画像サイズを表す
    // getSurfaceCapabilitiesKHRで得られたminImageExtent(最小値)とmaxImageExtent(最大値)の間でなければならない
    // currentExtentで現在のサイズが得られるため、それを指定
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    LOG("swapchainCreateInfo " << swapchainCreateInfo.imageExtent.width << ", " << swapchainCreateInfo.imageExtent.height);
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    // preTransformは表示時の画面反転・画面回転などのオプションを指定する
    // これもgetSurfaceCapabilitiesKHRの戻り値に依存する
    // supportedTransformsでサポートされている中に含まれるものでなければならないので、無難なcurrentTransform(現在の画面設定)を指定
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    // resentModeは表示処理のモードを示すもの
    // getSurfacePresentModesKHRの戻り値の配列に含まれる値である必要がある
    swapchainCreateInfo.presentMode = swapchainPresentMode;
    swapchainCreateInfo.clipped = VK_TRUE;

    // スワップチェーンは自分でイメージを保持している
    // 我々はイメージを作成する必要はなく、スワップチェーンが元から保持しているイメージを取得してそこに描画することになる
    //
    // スワップチェーンからイメージを取得した後は基本的に同じ
    // イメージからイメージビューを作成、
    // レンダーパス・パイプラインなどの作成、
    // フレームバッファを作成してイメージビューと紐づけ、
    // コマンドバッファにコマンドを積んでキューに送信
    vk::UniqueSwapchainKHR swapchain = device.createSwapchainKHRUnique(swapchainCreateInfo);

    std::vector<vk::Image> swapchainImages = device.getSwapchainImagesKHR(swapchain.get());
    std::vector<vk::UniqueImageView> swapchainImageViews(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++) 
    {
        vk::ImageViewCreateInfo imgViewCreateInfo;
        imgViewCreateInfo.image = swapchainImages[i];
        imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
        // ここに指定するフォーマットは元となるイメージに合わせる必要がある
        // スワップチェーンのイメージの場合、イメージのフォーマットはスワップチェーンを作成する時に指定したフォーマットになるため、その値を指定
        imgViewCreateInfo.format = swapchainFormat.format;
        imgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imgViewCreateInfo.subresourceRange.levelCount = 1;
        imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imgViewCreateInfo.subresourceRange.layerCount = 1;

        swapchainImageViews[i] = device.createImageViewUnique(imgViewCreateInfo);
    }

    // // Vulkanにおいて画像は vk::Image というオブジェクトで表される
    // // imageType には画像の次元を指定する
    // // 1次元から3次元まで指定できる
    // // 2次元以外の画像は、Rampテクスチャやvoxelなどで使用することがある
    // // 
    // // extent には画像のサイズを指定する
    // // ここでは640×480を指定(１は奥行き)
    // // 
    // // format には画素のフォーマットの種類を指定
    // // どのように各ピクセルが色を保存しているかの情報
    // // いろいろなフォーマットが存在するが、今回はひとまずR8G8B8A8Unormという形式を指定する
    // // RGB + A(透明度)をこの順でそれぞれ8bitで保存する形式
    // // 
    // // これで画像オブジェクトを作成することができた
    // // しかしこの段階ではまだ画像にメモリが割り当てられていない
    // // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VkImageCreateInfo
    // vk::ImageCreateInfo imgCreateInfo;
    // imgCreateInfo.imageType = vk::ImageType::e2D;
    // imgCreateInfo.extent = vk::Extent3D(screenWidth, screenHeight, 1);
    // imgCreateInfo.mipLevels = 1;
    // imgCreateInfo.arrayLayers = 1;
    // imgCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
    // imgCreateInfo.tiling = vk::ImageTiling::eLinear;
    // imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    // imgCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment;
    // imgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    // imgCreateInfo.samples = vk::SampleCountFlagBits::e1;

    // vk::UniqueImage image = device.createImageUnique(imgCreateInfo);

    // // イメージを使うためには、イメージにメモリを確保して割り当てる必要がある
    // // このとき使うメモリはnewやmallocで確保できる通常のメモリとは違う
    // // これから扱うのはデバイスメモリという特殊なメモリ
    // // 
    // // 実は、通常のメモリはGPUからアクセスすることができない
    // // そこでGPUからアクセスできる特殊なメモリを用意する必要がある
    // // それが「デバイスメモリ」
    // // 
    // // デバイスメモリはvk::Deviceの allocateMemory メソッドで確保する
    // // 確保したデバイスメモリは、bindImageMemoryメソッドで前節で作成したイメージに結びつけることができる
    // //
    // // new演算子やmallocなどで確保する通常のメモリとデバイスメモリの重要な違いとして、デバイスメモリには「種類」がある
    // // デバイスメモリを使う際はどれを使うか適切に選ばなければならない
    // // どんな種類のデバイスメモリがあるかは物理デバイス依存
    // // vk::PhysicalDeviceのgetMemoryPropertiesメソッドで取得できる

    // vk::PhysicalDeviceMemoryProperties構造体にはGPUが何種類のメモリを持っているかを表す、
    // memoryTypeCountメンバ変数、各種類のメモリに関する情報を表すmemoryTypesメンバ変数が格納されている
    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();
    LOG("----------------------------------------");
    LOG("memory type count: " << memProps.memoryTypeCount);
    LOG("memory heap count: " << memProps.memoryHeapCount);
    for (size_t i = 0; i < memProps.memoryTypeCount; i++)
    {
        LOG("----------------------------------------");
        LOG("memory type index: " << i);
        LOG(to_string(memProps.memoryTypes[i].propertyFlags));
    }

    // // イメージに対してどんな種類のメモリがどれだけ必要かという情報はvk::DeviceのgetImageMemoryRequirementsで取得できる
    // // vk::MemoryRequirements構造体には必要なメモリサイズを表すsizeメンバ変数、
    // // そして使用可能なメモリの種類をビットマスクで表すmemoryTypeBitsメンバ変数が格納されている
    // vk::MemoryRequirements imgMemReq = device.getImageMemoryRequirements(image.get());
    // LOG("----------------------------------------");
    // LOG("img size: " << imgMemReq.size);
    // LOG("img memoryTypeBits: " << imgMemReq.memoryTypeBits);

    // // 例えばmemoryTypeBitsの中身が2進数で0b00101011だった場合を考えてみる
    // // 右から0番目、1番目、3番目、5番目のビットが1になっているので、
    // // 使えるメモリの種類はメモリタイプ0番、1番、3番、5番となる
    // //
    // // メモリを確保するときはvk::MemoryAllocateInfoに必要なメモリのサイズと種類を指定する
    // //
    // // memoryTypeBitsを確認して、ビットが立っていればその番号のメモリタイプを使用する
    // // 今回は使えるメモリタイプの中から単純に一番最初に見つかったものを使用している
    // vk::MemoryAllocateInfo imgMemAllocInfo;
    // imgMemAllocInfo.allocationSize = imgMemReq.size;

    // bool foundSuitableMemoryType = false;
    // for (size_t i = 0; i < memProps.memoryTypeCount; i++)
    // {
    //     // あとで画像として出力しやすいようにホスト可視のメモリを選択する
    //     if (imgMemReq.memoryTypeBits & (1 << i)
    //      && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible))
    //     {
    //         imgMemAllocInfo.memoryTypeIndex = i;
    //         foundSuitableMemoryType = true;
    //         break;
    //     }
    // }

    // if (!foundSuitableMemoryType) 
    // {
    //     LOGERR("No memory are available");
    //     return EXIT_FAILURE;
    // }

    // vk::UniqueDeviceMemory imgMem = device.allocateMemoryUnique(imgMemAllocInfo);

    // // 第3引数の0は何かというと、「確保したメモリの先頭から何バイト目を使用するか」という項目である
    // // 例えば、メモリ1000バイトを要するイメージAとメモリ1500バイトを要するイメージBの2つがあった時、
    // // 2500バイトを確保して先頭から0バイト目以降をイメージAに、1000バイト目以降をイメージBにバインドするといったことができる
    // // ここでは普通に0を指定
    // device.bindImageMemory(image.get(), imgMem.get(), 0);

    // // 実は「イメージ」はこのままだとパイプラインの処理の中からは扱うことができない
    // // パイプラインで描画の対象として扱うには、
    // // 「イメージ」から「イメージビュー」という繋ぎのためのオブジェクトを作ってやる必要がある
    // // レンダーパスの中のそれぞれのアタッチメントには「イメージ」ではなく、
    // // この節で作成する「イメージビュー」を結び付けることになる
    // vk::ImageViewCreateInfo imgViewCreateInfo;
    // // imageにはイメージビューが指すイメージ
    // imgViewCreateInfo.image = image.get();
    // // viewTypeはイメージビューの種別
    // // 今回は元となるイメージに合わせて2Dを指定する
    // imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
    // // formatにはイメージビューのフォーマット
    // // イメージの作成時に指定したフォーマットに合わせる
    // imgViewCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
    // imgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    // imgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    // imgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    // imgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
    // imgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    // imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
    // imgViewCreateInfo.subresourceRange.levelCount = 1;
    // imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    // imgViewCreateInfo.subresourceRange.layerCount = 1;
    // // Vulkanの仕様では一つのイメージオブジェクトが複数のレイヤーを持つことができる
    // // イメージビューによって、その中のどのレイヤーをアタッチメント(=描画対象)として使うのかといった部分を指定したりできる
    // // そうした部分をviewType, components, subresourceRangeなどで指定する
    // //
    // // 「イメージ」と「イメージビュー」の違いは、
    // // イメージがただの画像を表すオブジェクトなのに対し、
    // // イメージビューは「どのイメージを扱うか」と「そのイメージを描画処理の上でどのように扱うか」をひとまとめにしたオブジェクト
    // // と考えられる
    // // 色として扱うのかとか、2Dなのかどうかとか

    // vk::UniqueImageView imgView = device.createImageViewUnique(imgViewCreateInfo);

    // レンダーパス
    vk::AttachmentDescription attachments[1];
    // formatにはイメージのフォーマット情報を指定する必要がある
    attachments[0].format = swapchainFormat.format;
    attachments[0].samples = vk::SampleCountFlagBits::e1;
    attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
    attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
    attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[0].initialLayout = vk::ImageLayout::eUndefined;
    // イメージレイアウト
    // イメージのメモリ上における配置方法・取扱い方に関する指定
    // 以前は特に説明せずvk::ImageLayout::eGeneralを指定していた
    // 実に色々な設定があり、必要に応じて最適なものを指定しなければならない
    // 公式ドキュメントなどの資料を参考に正しいものを探すしかない
    // 間違ったものを指定するとエラーが出る場合がある
    // レンダーパスの設定によって、レンダリング処理が終わった後でどのようなイメージレイアウトにするかを決めることができる
    // 今回はレンダリングが終わった後で表示(プレゼン)しなければならず、その場合はvk::ImageLayout::ePresentSrcKHRでなければならないという決まりなのでこれを指定
    // ちなみにeGeneral形式のイメージは(最適ではないものの)一応どんな扱いを受けても基本的にはOKらしく、eGeneralのままでも動くには動く でもやめておいた方が無難
    attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference subpass0_attachmentRefs[1];
    subpass0_attachmentRefs[0].attachment = 0;
    subpass0_attachmentRefs[0].layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpasses[1];
    subpasses[0].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = subpass0_attachmentRefs;

    vk::RenderPassCreateInfo renderpassCreateInfo;
    renderpassCreateInfo.attachmentCount = 1;
    renderpassCreateInfo.pAttachments = attachments;
    renderpassCreateInfo.subpassCount = 1;
    renderpassCreateInfo.pSubpasses = subpasses;
    renderpassCreateInfo.dependencyCount = 0;
    renderpassCreateInfo.pDependencies = nullptr;

    vk::UniqueRenderPass renderpass = device.createRenderPassUnique(renderpassCreateInfo);

    // // 描画の対象となる場所ができたので、ここからは実際の描画処理を書く
    // // 描画をするにあたっては、まずレンダーパスというものが必要
    // // レンダーパスとは一言で言えば描画の処理順序を記述したオブジェクトで、Vulkanにおける描画処理の際には必ず必要になる
    // // 大ざっぱな計画書みたいなもので、コマンド(＝具体的な作業手順)とは別物

    // // レンダーパスの情報を構成するものは３つある
    // // 「アタッチメント」「サブパス」「サブパス依存性」
    // //
    // // アタッチメントはレンダーパスにおいて描画処理の対象となる画像データのこと
    // // 書き込まれたり読み込まれたりする
    // //
    // // サブパスは1つの描画処理のことで、単一または複数のサブパスが集まってレンダーパス全体を構成する
    // // サブパスは任意の個数のアタッチメントを入力として受け取り、任意の個数のアタッチメントに描画結果を出力する
    // //
    // // サブパス依存性はサブパス間の依存関係
    // // 「サブパス1番が終わってからでないとサブパス2番は実行できない」とかそういう関係を表す
    // //
    // // 有向グラフとして考えてみると分かりやすい
    // // 
    // // 複雑なレンダーパスで複雑な描画処理を表現することもできるが、
    // // 今回は1つのサブパスと1つのアタッチメントだけで構成される単純なレンダーパスを作成する

    // vk::AttachmentDescription attachments[1];
    // attachments[0].format = vk::Format::eR8G8B8A8Unorm;
    // attachments[0].samples = vk::SampleCountFlagBits::e1;
    // // attachments[0].loadOp = vk::AttachmentLoadOp::eDontCare;
    // attachments[0].loadOp = vk::AttachmentLoadOp::eClear;  
    // attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
    // attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    // attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    // attachments[0].initialLayout = vk::ImageLayout::eUndefined;
    // attachments[0].finalLayout = vk::ImageLayout::eGeneral;

    // // vk::AttachmentReference構造体のattachmentメンバは「何番のアタッチメント」という形で
    // // レンダーパスの中のアタッチメントを指定する
    // // ここでは0を指定しているので0番のアタッチメントの意味
    // vk::AttachmentReference subpass0_attachmentRefs[1];
    // subpass0_attachmentRefs[0].attachment = 0;
    // subpass0_attachmentRefs[0].layout = vk::ImageLayout::eColorAttachmentOptimal;

    // vk::SubpassDescription subpasses[1];
    // subpasses[0].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    // subpasses[0].colorAttachmentCount = std::size(subpass0_attachmentRefs);
    // subpasses[0].pColorAttachments = subpass0_attachmentRefs;

    // vk::RenderPassCreateInfo renderpassCreateInfo;
    // renderpassCreateInfo.attachmentCount = std::size(attachments);
    // renderpassCreateInfo.pAttachments = attachments;
    // renderpassCreateInfo.subpassCount = std::size(subpasses);
    // renderpassCreateInfo.pSubpasses = subpasses;
    // renderpassCreateInfo.dependencyCount = 0;
    // renderpassCreateInfo.pDependencies = nullptr;

    // // レンダーパスを作成
    // vk::UniqueRenderPass renderpass = device.createRenderPassUnique(renderpassCreateInfo);

    // // これでレンダーパスが作成できたが、
    // // レンダーパスはあくまで「この処理はこのデータを相手にする、あの処理はあのデータを～」
    // // という関係性を表す”枠組み”に過ぎず、それぞれの処理(＝サブパス)が具体的にどのような処理を行うかは関知しない
    // // 実際にはいろいろなコマンドを任意の回数呼ぶことができる
    
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

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;

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

    vk::PipelineLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.setLayoutCount = 0;
    layoutCreateInfo.pSetLayouts = nullptr;

    vk::UniquePipelineLayout pipelineLayout = device.createPipelineLayoutUnique(layoutCreateInfo);

    vk::UniqueShaderModule vertShader;
    vk::UniqueShaderModule fragShader;
    {
        // 頂点シェーダーを読み込む
        size_t vertSpvFileSz = std::filesystem::file_size("../src/shader.vert.spv");
        std::ifstream vertSpvFile = std::ifstream("../src/shader.vert.spv", std::ios_base::binary);
        std::vector<char> vertSpvFileData = std::vector<char>(vertSpvFileSz);
        vertSpvFile.read(vertSpvFileData.data(), vertSpvFileSz);

        vk::ShaderModuleCreateInfo vertShaderCreateInfo;
        vertShaderCreateInfo.codeSize = vertSpvFileSz;
        vertShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertSpvFileData.data());
        vertShader = device.createShaderModuleUnique(vertShaderCreateInfo);

        // フラグメントシェーダーを読み込む
        size_t fragSpvFileSz = std::filesystem::file_size("../src/shader.frag.spv");
        std::ifstream fragSpvFile = std::ifstream("../src/shader.frag.spv", std::ios_base::binary);
        std::vector<char> fragSpvFileData = std::vector<char>(fragSpvFileSz);
        fragSpvFile.read(fragSpvFileData.data(), fragSpvFileSz);

        vk::ShaderModuleCreateInfo fragShaderCreateInfo;
        fragShaderCreateInfo.codeSize = fragSpvFileSz;
        fragShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragSpvFileData.data());
        fragShader = device.createShaderModuleUnique(fragShaderCreateInfo);
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
    pipelineCreateInfo.layout = pipelineLayout.get();
    pipelineCreateInfo.renderPass = renderpass.get();
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.stageCount = std::size(shaderStage);
    pipelineCreateInfo.pStages = shaderStage;

    vk::UniquePipeline pipeline = device.createGraphicsPipelineUnique(nullptr, pipelineCreateInfo).value;

    // フレームバッファはレンダーパスのアタッチメントとイメージビューを対応付けるもの
    // 今回は画面へ表示するにあたり、スワップチェーンの各イメージビューに描画していく
    // ということは、フレームバッファは1つだけではなくイメージビューごとに作成する必要がある
    std::vector<vk::UniqueFramebuffer> swapchainFramebufs(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); i++) 
    {
        vk::ImageView frameBufAttachments[1];
        frameBufAttachments[0] = swapchainImageViews[i].get();

        vk::FramebufferCreateInfo frameBufCreateInfo;
        // currentExtentから画面サイズを取得
        // 後々画面のリサイズ処理に対応するときにこうしないと問題になる
        frameBufCreateInfo.width = surfaceCapabilities.currentExtent.width;
        frameBufCreateInfo.height = surfaceCapabilities.currentExtent.height;
        frameBufCreateInfo.layers = 1;
        frameBufCreateInfo.renderPass = renderpass.get();
        frameBufCreateInfo.attachmentCount = 1;
        frameBufCreateInfo.pAttachments = frameBufAttachments;

        swapchainFramebufs[i] = device.createFramebufferUnique(frameBufCreateInfo);
    }

    // // レンダーパスは処理(サブパス)とデータ(アタッチメント)のつながりと関係性を記述するが、具体的な処理内容やどのデータを扱うかについては関与しない
    // // 具体的な処理内容はコマンドバッファに積むコマンドやパイプラインによって決まるが、具体的なデータの方を決めるためのものがフレームバッファである
    
    // // イメージビューの情報を初期化用構造体に入れている
    // // これで0番のアタッチメントがどのイメージビューに対応しているのかを示すことができる
    // vk::ImageView frameBufAttachments[1];
    // frameBufAttachments[0] = imgView.get();

    // // フレームバッファを介して「0番のアタッチメントはこのイメージビュー、1番のアタッチメントは…」という結び付けを行うことで初めてレンダーパスが使える
    // vk::FramebufferCreateInfo frameBufCreateInfo;
    // frameBufCreateInfo.width = screenWidth;
    // frameBufCreateInfo.height = screenHeight;
    // frameBufCreateInfo.layers = 1;
    // frameBufCreateInfo.renderPass = renderpass.get();
    // frameBufCreateInfo.attachmentCount = 1;
    // frameBufCreateInfo.pAttachments = frameBufAttachments;

    // // ここで注意だが、初期化用構造体にレンダーパスの情報を入れてはいるものの、これでレンダーパスとイメージビューが結びついた訳ではない
    // // ここで入れているレンダーパスの情報はあくまで「このフレームバッファはどのレンダーパスと結びつけることができるのか」を表しているに過ぎず、
    // // フレームバッファを作成した時点で結びついた訳ではない
    // // フレームバッファとレンダーパスを本当に結びつける処理はこの次に行う
    // // 
    // // パイプラインの作成処理でもレンダーパスの情報を渡していますが、ここにも同じ事情がある
    // // フレームバッファとパイプラインは特定のレンダーパスに依存して作られるものであり、互換性のない他のレンダーパスのために働こうと思ってもそのようなことはできない
    // // 結びつけを行っている訳ではないのにレンダーパスの情報を渡さなければならないのはそのため
    // vk::UniqueFramebuffer frameBuf = device.createFramebufferUnique(frameBufCreateInfo);

    // もしも固定の内容のコマンドバッファを使ってしまうと、毎回同じフレームバッファが使われることになる
    // 毎回同じフレームバッファということは、毎回同じイメージに向けて描画することになってしまう
    // これではスワップチェーンによる表示処理がうまくいかない
    // そこで毎フレームコマンドバッファをリセットして、コマンドを記録し直す こうすることで毎回別のイメージに向けて描画できる
    vk::CommandPoolCreateInfo cmdPoolCreateInfo;
    cmdPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    // コマンドプール作成時 vk::CommandPoolCreateInfo::flags に vk::CommandPoolCreateFlagBits::eResetCommandBuffer を指定すると、
    // そのコマンドプールから作成したコマンドバッファはリセット可能になる
    cmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    vk::UniqueCommandPool cmdPool = device.createCommandPoolUnique(cmdPoolCreateInfo);

    // // コマンドバッファを作るには、その前段階として「コマンドプール」というまた別のオブジェクトを作る必要がある
    // // コマンドバッファをコマンドの記録に使うオブジェクトとすれば、コマンドプールというのはコマンドを記録するためのメモリ実体
    // // コマンドバッファを作る際には必ず必要
    // // コマンドプールは論理デバイス(vk::Device)の createCommandPool メソッド、コマンドバッファは論理デバイス(vk::Device)の allocateCommandBuffersメソッドで作成することができる
    // // コマンドプールの作成が「create」なのに対し、コマンドバッファの作成は「allocate」であるあたりから
    // // 「コマンドバッファの記録能力は既に用意したコマンドプールから割り当てる」という気持ちが読み取れる
    // // 
    // // CommandPoolCreateInfoのqueueFamilyIndexには、後でそのコマンドバッファを送信するときに対象とするキューを指定する
    // // 結局送信するときにも「このコマンドバッファをこのキューに送信する」というのは指定するが、こんな二度手間が盛り沢山なのがVulkanである
    // // 次はコマンドバッファを作成する
    // // allocateCommandBufferではなくallocateCommandBuffersである名前から分かる通り、一度にいくつも作れる仕様になっている
    // vk::CommandPoolCreateInfo cmdPoolCreateInfo;
    // cmdPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    // vk::UniqueCommandPool cmdPool = device.createCommandPoolUnique(cmdPoolCreateInfo);

    // コマンドバッファの作成
    // コマンドバッファを毎フレーム記録しなおせるようにするのは、描画対象のイメージを切り替えるという以外にも意味がある。
    // 今は単なる三角形の表示ですが、動くアニメーションになれば毎フレーム描画内容が変わる
    // そこでフレーム毎にコマンド(描画命令)を変えるのは自然なこと
    vk::CommandBufferAllocateInfo cmdBufAllocInfo;
    cmdBufAllocInfo.commandPool = cmdPool.get();
    cmdBufAllocInfo.commandBufferCount = 1;
    cmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> cmdBufs = device.allocateCommandBuffersUnique(cmdBufAllocInfo);

    // // コマンドバッファとは、コマンドをため込んでおくバッファ
    // // VulkanでGPUに仕事をさせる際は「コマンドバッファの中にコマンドを記録し、そのコマンドバッファをキューに送る」必要がある
    // // 
    // // 作るコマンドバッファの数はCommandBufferAllocateInfoの commandBufferCount で指定する
    // // commandPoolにはコマンドバッファの作成に使うコマンドプールを指定する
    // // このコードではUniqueCommandPoolを使っているので.get()を呼び出して生のCommandPoolを取得している
    // vk::CommandBufferAllocateInfo cmdBufAllocInfo;
    // cmdBufAllocInfo.commandPool = cmdPool.get();
    // cmdBufAllocInfo.commandBufferCount = 1;
    // cmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    // std::vector<vk::UniqueCommandBuffer> cmdBufs = device.allocateCommandBuffersUnique(cmdBufAllocInfo);

    vk::FenceCreateInfo fenceCreateInfo;
    vk::UniqueFence swapchainImgFence = device.createFenceUnique(fenceCreateInfo);

    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
        
        device.resetFences({ swapchainImgFence.get() });
    
        // スワップチェーンを利用する私たちの側としては「次に表示される予定のイメージ」に描画を行いたい
        // そこで「次に描画を行うべきイメージ」を、論理デバイスのacquireNextImageKHRメソッドで取得
        // 前回スワップチェーンのイメージを配列として取得したが、その何番目のイメージという番号の形で手に入る
        // このメソッドは、次に描画するべきイメージを教えてくれるだけでなく「描画されてもいいように準備する処理」なども含んでいるため、
        // スワップチェーンのイメージに描画する前には必ず呼ぶ必要がある
        //
        // 正確には「描画された後表示されてもいいように準備する処理」
        //
        // acquireNextImageKHRの第二引数はタイムアウト時間(最大待ち時間)
        // 単位はナノ秒で、ここでは1'000'000'000ナノ秒=1秒としている
        // UINT64_MAXを指定した場合は無制限に待つ

        // もう一つ重要なのが、「フェンス」を使って同期処理を行っているところ
        // 実はこのacquireNextImageKHR、ただちに描画の準備を整えてくれるわけではない
        // キューにコマンドを送信した時などと同じように、少し待たないと描画の準備を完了してくれない
        // いわゆる非同期処理
        // (イメージのインデックスだけ返り値として先に返してくれる)

        // コマンド送信の時はwaitIdle()などを使っていましたが今回はキューの処理ではないので、
        // フェンスを利用して描画の準備が終わるまで待機している

        vk::ResultValue acquireImgResult = device.acquireNextImageKHR(swapchain.get(), 1'000'000'000, {}, swapchainImgFence.get());
        if (acquireImgResult.result != vk::Result::eSuccess) 
        {
            LOG("Failed to get next frame");
            return EXIT_FAILURE;
        }
        uint32_t imgIndex = acquireImgResult.value;
    
        if (device.waitForFences({ swapchainImgFence.get() }, VK_TRUE, 1'000'000'000) != vk::Result::eSuccess)
        {
            LOG("Failed to get next frame");
            return EXIT_FAILURE;
        }
    
        // コマンドバッファのリセット
        cmdBufs[0]->reset();
    
        // コマンドの記録
        vk::CommandBufferBeginInfo cmdBeginInfo;
        cmdBufs[0]->begin(cmdBeginInfo);
        
        vk::ClearValue clearVal[1];
        clearVal[0].color.float32[0] = 0.0f;
        clearVal[0].color.float32[1] = 0.0f;
        clearVal[0].color.float32[2] = 0.0f;
        clearVal[0].color.float32[3] = 1.0f;

        vk::RenderPassBeginInfo renderpassBeginInfo;
        renderpassBeginInfo.renderPass = renderpass.get();
        renderpassBeginInfo.framebuffer = swapchainFramebufs[imgIndex].get();
        renderpassBeginInfo.renderArea = vk::Rect2D({ 0,0 }, surfaceCapabilities.currentExtent);
        renderpassBeginInfo.clearValueCount = 1;
        renderpassBeginInfo.pClearValues = clearVal;

        cmdBufs[0]->beginRenderPass(renderpassBeginInfo, vk::SubpassContents::eInline);

        cmdBufs[0]->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
        cmdBufs[0]->draw(3, 1, 0, 0);
    
        cmdBufs[0]->endRenderPass();

        cmdBufs[0]->end();
        
        // コマンドバッファの送信
        vk::CommandBuffer submitCmdBuf[1] = { cmdBufs[0].get() };
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = submitCmdBuf;
        graphicsQueue.submit({ submitInfo }, nullptr);
        graphicsQueue.waitIdle();
    
        // 表示処理(後で説明)
        vk::PresentInfoKHR presentInfo;

        auto presentSwapchains = { swapchain.get() };
        auto imgIndices = { imgIndex };
        
        presentInfo.swapchainCount = presentSwapchains.size();
        presentInfo.pSwapchains = presentSwapchains.begin();
        presentInfo.pImageIndices = imgIndices.begin();
        
        graphicsQueue.presentKHR(presentInfo);
        
        graphicsQueue.waitIdle();
    }

    glfwTerminate();

    // // レンダーパスの開始と終了を指示するコマンドを送る
    // // Vulkanにおける描画処理は必ずレンダーパスの枠組みで行う必要がある
    // // コマンドバッファのbeginRenderPassメソッドとendRenderPassメソッドでそれぞれレンダーパスの開始と終了を行うコマンドを記録できる

    // // コマンドを記録
    // vk::CommandBufferBeginInfo cmdBeginInfo;
    // cmdBufs[0]->begin(cmdBeginInfo);
    // {
    //     vk::ClearValue clearVals[1];
    //     clearVals[0].color = vk::ClearColorValue(0, 0, 0, 1);

    //     vk::RenderPassBeginInfo renderpassBeginInfo;
    //     renderpassBeginInfo.renderPass = renderpass.get();
    //     renderpassBeginInfo.framebuffer = frameBuf.get();
    //     renderpassBeginInfo.renderArea = vk::Rect2D({ 0,0 }, { screenWidth, screenHeight });
    //     renderpassBeginInfo.clearValueCount = std::size(clearVals);
    //     renderpassBeginInfo.pClearValues = clearVals;

    //     // 描画処理は、どのパイプラインを使って行うかを示す必要がある
    //     // これはbindPipelineというコマンドで行う
    //     cmdBufs[0]->beginRenderPass(renderpassBeginInfo, vk::SubpassContents::eInline);
    //     {
    //         // ここでサブパス0番の処理
    //         cmdBufs[0]->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());

    //         // 全てはこれをやるための道のり！
    //         cmdBufs[0]->draw(3, 1, 0, 0);
    //         // 第1引数は頂点の数、これが3つということは頂点シェーダは3回呼び出され三角形が1つ描かれることを意味する
    //         // 第2引数はインスタンスの数
    //         // 複数のインスタンスを扱うと同じ格好をした物体を一度に複数まとめて描画できる
    //         // 第3引数・第4引数は頂点のオフセットとインスタンスのオフセット
    //         // 例えば頂点の情報が2000個分あって、前の1000頂点が物体Aのモデルデータを表し後の1000頂点が物体Bのモデルデータを表すといった場合、
    //         // 第3引数に1000を渡すことで物体Bのモデル描画に使うことができる
    //     }

    //     // レンダーパスを開始した直後に何らかのコマンドを送ればサブパス0番の中の処理という扱いになる
    //     // ここではサブパス1個だけのレンダーパスを使っているため、そのままレンダーパスを終える
    //     // なお、複数のサブパスを持つレンダーパスを用いる場合は以下のようにnextSubpassというコマンドで次のサブパスへ移行する

    //     // cmdBufs[0]->nextSubpass(vk::SubpassContents::eInline);
    //     // サブパス1番の処理
    //     // cmdBufs[0]->nextSubpass(vk::SubpassContents::eInline);
    //     // サブパス2番の処理
    //     // cmdBufs[0]->endRenderPass();

    //     cmdBufs[0]->endRenderPass();
    // }
    // cmdBufs[0]->end();

    // // コマンドの記録が終わったらそれをキューに送信
    // // vk::SubmitInfo構造体にコマンドバッファの送信に関わる情報を指定する
    // // commandBufferCountには送信するコマンドバッファの数
    // // pCommandBuffersには送信するコマンドバッファの配列へのポインタを指定
    // vk::CommandBuffer submitCmdBuf[1] = { cmdBufs[0].get() };
    // vk::SubmitInfo submitInfo;
    // submitInfo.commandBufferCount = std::size(submitCmdBuf);
    // submitInfo.pCommandBuffers = submitCmdBuf;

    // // キューは論理デバイス(vk::Device)の getQueue メソッドで取得できる
    // // 第1引数がキューファミリのインデックス、第2引数が取得したいキューのキューファミリ内でのインデックス
    // vk::Queue graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);

    // // 送信はvk::Queueのsubmitメソッドで行う
    // // 第2引数は現在nullptrにしているが、本来はフェンスというものを指定することができる
    // // こうしてsubmitでGPUに命令を送ることが出来るが、あくまで「送る」だけである
    // // キューに仕事を詰め込むだけであるため、submitから処理が返ってきた段階で送った命令が完了されているとは限らない
    // std::vector<vk::SubmitInfo> submits;
    // submits.push_back(submitInfo);
    // graphicsQueue.submit(submits, nullptr);

    // // 実用的なプログラムを作るためには、「依頼した処理が終わるまで待つ」方法を知る必要がある
    // // これに必要なのが「セマフォ」や「フェンス」
    // // とりあえずここでは、単純に「キューが空になるまで待つ」方法を書く
    // // 
    // // キューのwaitIdleメソッドを呼ぶと、その段階でキューに入っているコマンドが全て実行完了してキューが空になるまで待つことができる
    // graphicsQueue.waitIdle();

    // // データはメインメモリではなくGPUの中にある
    // // つまりこのままではアプリケーションからアクセスできない
    // // よって、アプリケーション側のアドレス空間上にマップして、メインのプログラムから見れるようにする必要がある
    // //
    // // GPU上のメモリには種類があり、適切に選択する必要がある
    // // 実はGPU上のメモリには、GPUを操作しているホストからアクセスできるものとアクセスできないものがある
    // // そこで、メモリを確保するところで「目的のイメージにバインドできるメモリ」であるだけでなく「ホスト側から見えるメモリ」であるものを選択する必要があります。
    // void* imgData = device.mapMemory(imgMem.get(), 0, imgMemReq.size);

    // // 引数はそれぞれファイル名、幅、高さ、1画素当たりのバイト数、データ
    // // 第4引数が4のとき、それぞれの画素はRGBAをこの順でそれぞれ1バイト(符号なし8ビット)で表す
    // stbi_write_bmp("img.bmp", screenWidth, screenHeight, 4, imgData);
    // device.unmapMemory(imgMem.get());

    return EXIT_SUCCESS;
}

void WaitPrograms(vk::Device device, vk::Queue graphicsQueue, std::vector<vk::SubmitInfo> submits, vk::SubmitInfo submitInfo)
{
    // セマフォなどを使うと送った個別のコマンドに関する待ち処理などが出来る
    // GPUに投げた処理は基本的に非同期処理になる
    // そのため、レンダリングなどの処理をGPUに投げた場合、状況に応じて処理の完了を待つ必要がある
    // このための機構が「フェンス」や「セマフォ」である
    // これらの機構を適切に利用することで、処理の順序関係がおかしなことにならないで済む
    // 
    // フェンスもセマフォもGPUの処理を待つための機構であることに違いない
    // フェンスがホスト側で待機するための機構
    // セマフォは主にGPU内で他の処理を待機するための機構
    //
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 
    // フェンス(ホスト側で待機)
    // 論理デバイスのcreateFenceメソッドでフェンスオブジェクトを作成できる
    // フェンスオブジェクトは内部的に「シグナル状態」と「非シグナル状態」の2状態を持っている
    // 初期状態(あるいはリセット直後の状態)は非シグナル状態
    // 
    // Vulkanでは様々な処理に対して「この処理が終わったらこのフェンスをシグナル状態にしてね」と設定することができ、
    // 論理デバイスのwaitForFencesメソッドでシグナル状態になるまで待機することができる
    // シグナル状態になったフェンスは、resetFencesメソッドで非シグナル状態にリセットできる
    // 例えると、非シグナル状態は赤信号、シグナル状態は青信号
    vk::FenceCreateInfo fenceCreateInfo;
    vk::UniqueFence fence = device.createFenceUnique(fenceCreateInfo);

    // 特定の処理に対して「終わったらシグナル状態にするフェンス」を設定する方法
    // GPUを利用するほとんどの関数は引数としてフェンスを渡せる
    // 例として、キューのsubmitメソッドの第二引数にフェンスを渡すことができる
    // waitForFencesで待機するときはこのように書く
    graphicsQueue.submit(submits, fence.get());
    std::vector<vk::Fence> fences;
    fences.push_back(fence.get());

    // 第一引数に待機するフェンスを設定できる
    //  複数指定することができる
    // 第二引数は待機方法を指定する
    //  TRUEの場合、「全てのフェンスがシグナル状態になるまで待機」となる
    //  FALSEの場合、「どれか一つのフェンスがシグナル状態になるまで待機」となる
    // 第三引数は最大待機時間
    //  ナノ秒で指定
    //  1秒 = 10 ^ 9ナノ秒
    //  Vulkanで時間を指定する場合はナノ秒で指定することが多いので覚えておくと良い
    //  最大待機時間を超えて処理が終わらなかった場合、vk::Result::eTimeoutが返ってくる
    //  本シリーズではあまり真面目に対処していないが、ちゃんとしたアプリケーションを作る場合は考慮すると良い
    //  
    //  正常に終わった場合はvk::Result::eSuccessが返ってくる
    //  resetFencesで非シグナル状態に戻る
    //  これで再びフェンスが使えるようになる
    //  処理開始の直前、もしくは処理待ちの終了直後などにリセットすると良い
    vk::Result result = device.waitForFences(fences, VK_TRUE, 1'000'000'000);
    // これでGPUに投げた処理を待機できる

    // フェンスを初期状態でシグナル状態にしておきたい場合はvk::FenceCreateInfo::flagsにvk::FenceCreateFlagBits::eSignaledを設定する
    // vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    fence = device.createFenceUnique(fenceCreateInfo);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 
    // セマフォ(GPU内で他の処理を待機するための機構)
    // 作の基本はフェンスとそこまで変わらない
    // こちらもシグナル状態と非シグナル状態の2状態があり、シグナル状態になるまで待機することができる
    // タイムラインセマフォという、数値カウンターを持つ特殊で高機能なセマフォもある
    // Vulkanでは2状態のバイナリーセマフォの方が基本的
    // 論理デバイスのcreateSemaphoreメソッドでセマフォオブジェクトを作成できる
    // 
    // フェンスと違い、特定の処理の完了に引っ掛けるには構造体に指定することが多い(例外もあり)
    // 例としてコマンドバッファの送信時は、vk::SubmitInfo構造体のsignalSemaphoreCount、pSignalSemaphoresを用いる
    // ここに指定すると、送信したコマンドの処理が完了した時に指定したセマフォがシグナル状態になる
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    vk::UniqueSemaphore semaphore = device.createSemaphoreUnique(semaphoreCreateInfo);

    vk::Semaphore signalSemaphores[] = { semaphore.get() };
    submitInfo.signalSemaphoreCount = std::size(signalSemaphores);
    submitInfo.pSignalSemaphores = signalSemaphores;
    submits.push_back(submitInfo);
    graphicsQueue.submit(submits);

    // また、待機のために指定する場合も構造体に指定する
    // 最初に説明したように、セマフォはGPU内で他の処理を待機するために使われることに注意
    // コマンドバッファ送信時は、vk::SubmitInfo構造体のwaitSemaphoreCount、pWaitSemaphores、pWaitDstStageMaskで待機するセマフォを指定する
    vk::Semaphore waitSemaphores[] = { semaphore.get() };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eAllCommands };

    // pWaitDstStageMaskはパイプラインのどの時点で待機するかを指定するもの
    // 単純に処理の開始時点で待機するということもでき、これでも特に動作に問題はない
    // しかし例えば、フラグメントシェーダでだけ特定の処理の結果を待つ必要があるならば、
    // 「フラグメントシェーダの処理が始まる時点で待機」ということにして
    // 頂点シェーダの処理などはシグナル状態を待たずに始める、といったことができる
    // GPUの並列化能力に余裕がある場合はこのように、必要最低限の部分でだけ処理待ちをする方が効率が上がる
    submitInfo.waitSemaphoreCount = std::size(waitSemaphores);
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    // セマフォはフェンスと違い、明示的にリセットする必要はない
    // そのセマフォのシグナル状態を待っていた処理が開始すると、自動でリセットされる
    // こうした動作をするので、ある1つの処理に複数の他の処理が依存するという場合はその数だけセマフォが必要
    // 1つのセマフォのシグナルを複数の処理で待つことは一般的ではないため、フェンスのように信号機に例えて考えるのは適切ではない
    // セマフォとは「ある1つの処理の完了」と「ある1つの処理の開始」の順序関係の取り決めであり、1対1の関係を表す
    // 
    // 同期のための機構は実は他にもいろいろある
    // パイプラインバリアやイベント、また本編で扱ったレンダーパスもその1つ
    // しかしフェンスとセマフォが一番基本的で分かりやすく、また使う機会も多い
}