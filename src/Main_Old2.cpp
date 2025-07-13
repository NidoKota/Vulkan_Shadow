#include <vulkan/vulkan.hpp>
#include "Utility.hpp"

using namespace Vulkan_Test;

const char *requiredLayers[] = { "VK_LAYER_KHRONOS_validation" };

// https://chaosplant.tech/do/vulkan/

// cd build
// ."C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
// VisualStudioでビルド

int main()
{
    // これに色々情報を乗っけることで
    // 例えばVulkanの拡張機能をオンにしたりだとかデバッグ情報を表示させるといったことができる
    // これはオプション的なもの
    vk::InstanceCreateInfo instCreateInfo;
    instCreateInfo.enabledLayerCount = std::size(requiredLayers);
    instCreateInfo.ppEnabledLayerNames = requiredLayers;

    // Vulkanインスタンスを作ってから壊すまでの間に色々な処理を書いていく
    // Vulkanを使う場合、全ての機能はこのインスタンスを介して利用する
    vk::UniqueInstance instancePtr = vk::createInstanceUnique(instCreateInfo);
    vk::Instance instance = instancePtr.get();

    // GPUが複数あるなら頼む相手をまず選ぶ必要がある
    // GPUの型式などによってサポートしている機能とサポートしていない機能があったりするため、
    // だいたい「インスタンスを介して物理デバイスを列挙する」→「それぞれの物理デバイスの情報を取得する」→「一番いいのを頼む」という流れになる
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    for (vk::PhysicalDevice physicalDevice : physicalDevices)
    {
        vk::PhysicalDeviceProperties props = physicalDevice.getProperties();
        LOG("----------------------------------------");
        LOG(props.deviceName);
    }

    vk::PhysicalDevice physicalDevice;
    bool existsSuitablePhysicalDevice = false;
    uint32_t graphicsQueueFamilyIndex;
    for (size_t i = 0; i < physicalDevices.size(); i++)
    {
        std::vector<vk::QueueFamilyProperties> queueProps = physicalDevices[i].getQueueFamilyProperties();
        bool existsGraphicsQueue = false;

        for (size_t j = 0; j < queueProps.size(); j++)
        {
            if (queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics)
            {
                existsGraphicsQueue = true;
                graphicsQueueFamilyIndex = j;
                break;
            }
        }

        if (existsGraphicsQueue)
        {
            physicalDevice = physicalDevices[i];
            existsSuitablePhysicalDevice = true;
            break;
        }
    }

    if (!existsSuitablePhysicalDevice)
    {
        std::cerr << "No physical devices are available." << std::endl;
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
        LOG("graphic support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics ? "True" : "False"));
        LOG("compute support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eCompute ? "True" : "False"));
        LOG("transfer support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eTransfer ? "True" : "False"));
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
    vk::DeviceCreateInfo devCreateInfo;
    devCreateInfo.enabledLayerCount = std::size(requiredLayers);
    devCreateInfo.ppEnabledLayerNames = requiredLayers;
    devCreateInfo.pQueueCreateInfos = queueCreateInfo;
    devCreateInfo.queueCreateInfoCount = std::size(queueCreateInfo);

    // vk::Instanceにはそれに対応するvk::UniqueInstanceが存在しましたが、
    // vk::PhysicalDeviceに対応するvk::UniquePhysicalDevice は存在しない
    // destroyなどを呼ぶ必要もない
    // vk::PhysicalDeviceは単に物理的なデバイスの情報を表しているに過ぎないので、構築したり破棄したりする必要がある類のオブジェクトではない
    vk::UniqueDevice devicePtr = physicalDevice.createDeviceUnique(devCreateInfo);
    vk::Device device = devicePtr.get();

    // キューは論理デバイス(vk::Device)の getQueue メソッドで取得できる
    // 第1引数がキューファミリのインデックス、第2引数が取得したいキューのキューファミリ内でのインデックス
    vk::Queue graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);

    // 
    const uint32_t screenWidth = 640;
    const uint32_t screenHeight = 480;

    vk::ImageCreateInfo imgCreateInfo;
    imgCreateInfo.imageType = vk::ImageType::e2D;
    imgCreateInfo.extent = vk::Extent3D(screenWidth, screenHeight, 1);
    imgCreateInfo.mipLevels = 1;
    imgCreateInfo.arrayLayers = 1;
    imgCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
    imgCreateInfo.tiling = vk::ImageTiling::eLinear;
    imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    imgCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment;
    imgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imgCreateInfo.samples = vk::SampleCountFlagBits::e1;

    vk::UniqueImage image = device.createImageUnique(imgCreateInfo);
    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    // コマンドバッファを作るには、その前段階として「コマンドプール」というまた別のオブジェクトを作る必要がある
    // コマンドバッファをコマンドの記録に使うオブジェクトとすれば、コマンドプールというのはコマンドを記録するためのメモリ実体
    // コマンドバッファを作る際には必ず必要
    // コマンドプールは論理デバイス(vk::Device)の createCommandPool メソッド、コマンドバッファは論理デバイス(vk::Device)の allocateCommandBuffersメソッドで作成することができる
    // コマンドプールの作成が「create」なのに対し、コマンドバッファの作成は「allocate」であるあたりから
    // 「コマンドバッファの記録能力は既に用意したコマンドプールから割り当てる」という気持ちが読み取れる
    // 
    // CommandPoolCreateInfoのqueueFamilyIndexには、後でそのコマンドバッファを送信するときに対象とするキューを指定します。
    // 結局送信するときにも「このコマンドバッファをこのキューに送信する」というのは指定するのですが、
    // そこはあまり深く突っ込まないでください。こんな二度手間が盛り沢山なのがVulkanです。
    // 次はコマンドバッファを作成します。
    // allocateCommandBufferではなくallocateCommandBuffers である名前から分かる通り、一度にいくつも作れる仕様になっている
    vk::CommandPoolCreateInfo cmdPoolCreateInfo;
    cmdPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    vk::UniqueCommandPool cmdPool = device.createCommandPoolUnique(cmdPoolCreateInfo);

    // コマンドバッファとは、コマンドをため込んでおくバッファ
    // VulkanでGPUに仕事をさせる際は「コマンドバッファの中にコマンドを記録し、そのコマンドバッファをキューに送る」必要がある
    // 
    // 作るコマンドバッファの数はCommandBufferAllocateInfoの commandBufferCount で指定する
    // commandPoolにはコマンドバッファの作成に使うコマンドプールを指定する
    // このコードではUniqueCommandPoolを使っているので.get()を呼び出して生のCommandPoolを取得している
    vk::CommandBufferAllocateInfo cmdBufAllocInfo;
    cmdBufAllocInfo.commandPool = cmdPool.get();
    cmdBufAllocInfo.commandBufferCount = 1;
    cmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> cmdBufs = device.allocateCommandBuffersUnique(cmdBufAllocInfo);
    vk::MemoryRequirements imgMemReq = device.getImageMemoryRequirements(image.get());

    // コマンドを記録
    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBufs[0]->begin(cmdBeginInfo);

    cmdBufs[0]->end();

    // コマンドの記録が終わったらそれをキューに送信
    // vk::SubmitInfo構造体にコマンドバッファの送信に関わる情報を指定する
    // commandBufferCountには送信するコマンドバッファの数
    // pCommandBuffersには送信するコマンドバッファの配列へのポインタを指定
    vk::CommandBuffer submitCmdBuf[1] = { cmdBufs[0].get() };
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = std::size(submitCmdBuf);
    submitInfo.pCommandBuffers = submitCmdBuf;

    // 送信はvk::Queueの submit メソッドで行う
    // 第2引数は現在nullptrにしているが、本来はフェンスというものを指定することができる
    // こうしてsubmitでGPUに命令を送ることが出来るが、あくまで「送る」だけである
    // キューに仕事を詰め込むだけであるため、submitから処理が返ってきた段階で送った命令が完了されているとは限らない
    std::vector<vk::SubmitInfo> submits;
    submits.push_back(submitInfo);
    graphicsQueue.submit(submits, nullptr);

    // 実用的なプログラムを作るためには、「依頼した処理が終わるまで待つ」方法を知る必要がある
    // これに必要なのが「セマフォ」や「フェンス」
    // とりあえずここでは、単純に「キューが空になるまで待つ」方法を書く
    // 
    // キューのwaitIdleメソッドを呼ぶと、その段階でキューに入っているコマンドが全て実行完了してキューが空になるまで待つことができる
    graphicsQueue.waitIdle();

    // セマフォなどを使うと送った個別のコマンドに関する待ち処理などが出来る

    return EXIT_SUCCESS;
}