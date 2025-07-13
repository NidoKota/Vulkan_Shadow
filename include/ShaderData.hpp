#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

struct Vec2 {
    float x, y;
};

struct Vec3 {
    float x, y, z;
};

struct Mat4x4 {
    float v[4][4];
};

struct Vertex {
    Vec3 pos;
    Vec3 color;
    Vec2 texUV;
};

struct SceneData {
    Mat4x4 mvpMatrix[2];
};

struct ObjectData {
    int id;
};

Mat4x4 operator*(const Mat4x4 &a, const Mat4x4 &b) {
    Mat4x4 c = {};
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            for(int k = 0; k < 4; k++)
                c.v[i][j] += a.v[k][j] * b.v[i][k];
    return c;
}

std::vector<Vertex> vertices = {
    Vertex{Vec3{-0.5f, -0.5f, 0.5f}, Vec3{0.0, 0.0, 1.0}, Vec2{0.0, 0.0}},
    Vertex{Vec3{0.5f, 0.5f, 0.5f}, Vec3{0.0, 1.0, 0.0}, Vec2{1.0, 1.0}},
    Vertex{Vec3{0.5f, -0.5f, 0.5f}, Vec3{1.0, 1.0, 1.0}, Vec2{0.0, 1.0}},
    Vertex{Vec3{-0.5f, 0.5f, 0.5f}, Vec3{1.0, 0.0, 0.0}, Vec2{1.0, 0.0}},
    
    Vertex{Vec3{-0.5f, -0.5f, -0.5f}, Vec3{0.0, 0.0, 1.0}, Vec2{0.0, 0.0}},
    Vertex{Vec3{0.5f, 0.5f, -0.5f}, Vec3{0.0, 1.0, 0.0}, Vec2{1.0, 1.0}},
    Vertex{Vec3{0.5f, -0.5f, -0.5f}, Vec3{1.0, 1.0, 1.0}, Vec2{0.0, 1.0}},
    Vertex{Vec3{-0.5f, 0.5f, -0.5f}, Vec3{1.0, 0.0, 0.0}, Vec2{1.0, 0.0}},
};

std::vector<uint16_t> indices = {
    0, 1, 2, 1, 0, 3, 5, 4, 6, 4, 5, 7, 
    4, 3, 0, 3, 4, 7, 1, 6, 2, 6, 1, 5,
    7, 1, 3, 1, 7, 5, 6, 0, 2, 0, 6, 4
};

SceneData sceneData;
ObjectData objectData;

Mat4x4 scaleMatrix(float scale) 
{
    return Mat4x4{{
        {scale, 0, 0, 0},
        {0, scale, 0, 0},
        {0, 0, scale, 0},
        {0, 0, 0, 1},
    }};
}

Mat4x4 rotationMatrix(Vec3 n, float theta) 
{
    float c = cos(theta);
    float s = sin(theta);
    float nc = 1 - c;

    return Mat4x4{{
        {n.x * n.x * nc + c,       n.x * n.y * nc + n.z * s, n.x * n.z * nc - n.y * s, 0},
        {n.y * n.x * nc - n.z * s, n.y * n.y * nc + c,       n.y * n.z * nc + n.x * s, 0},
        {n.z * n.x * nc + n.y * s, n.z * n.y * nc - n.x * s, n.z * n.z * nc + c,       0},
        {0, 0, 0, 1},
    }};
}

Mat4x4 translationMatrix(Vec3 v) 
{
    return Mat4x4{{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {v.x, v.y, v.z, 1},
    }};
}

Mat4x4 viewMatrix(Vec3 cameraPos, Vec3 dir, Vec3 up) 
{
    const auto cameraShift = 
        Mat4x4{{
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {-cameraPos.x, -cameraPos.y, -cameraPos.z, 1},
        }};
    const auto cameraRotation = 
        Mat4x4{{
            {up.z * dir.y - up.y * dir.z, -up.x, dir.x, 0},
            {up.x * dir.z - up.z * dir.x, -up.y, dir.y, 0},
            {up.y * dir.x - up.x * dir.y, -up.z, dir.z, 0},
            {0, 0, 0, 1},
        }};

    return cameraRotation * cameraShift;
}

Mat4x4 projectionMatrix(float angle_y, float ratio, float near, float far) 
{
    float ky = tan(angle_y);
    float kx = ky * ratio;

    return Mat4x4{{
        {kx, 0, 0, 0},
        {0, ky, 0, 0},
        {0, 0, far/(far-near), 1},
        {0, 0, -near*far/(far-near), 0}
    }};
}

std::shared_ptr<vk::UniqueBuffer> getVertexBuffer(vk::UniqueDevice& device)
{
    // バッファというのはデバイスメモリ上のデータ列を表すオブジェクト
    // 何度も言うようにGPUから普通のメモリの内容は参照できない
    // シェーダで使いたいデータがある場合、まずデバイスメモリに移す必要がある
    // そしてそれはプログラムの上では「バッファに書き込む」「バッファに書き込んで転送する」という形になる
    // 頂点座標のデータをシェーダに送るためのバッファを作成する
    // いわゆる頂点バッファ
    // size はバッファの大きさをバイト数で示すもの
    // ここに例えば100という値を指定すれば、100バイトの大きさのバッファが作成できる
    // ここでは前節で定義した構造体のバイト数をsizeof演算子で取得し、それにデータの数をかけている
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();

    // 次は実際に使われる頂点バッファの作成
    // 今までと違い、メモリの確保時にvk::MemoryPropertyFlagBits::eDeviceLocalフラグを持ったメモリを使うようにするE
    // 逆にeHostVisibleは要らない
    // また、usageにvk::BufferUsageFlagBits::eTransferDstを追加で指定する
    // データの転送先という意味
    // あとでステージングバッファからデータを転送してくる
    vk::BufferCreateInfo vertexBufferCreateInfo;
    vertexBufferCreateInfo.size = sizeof(Vertex) * vertices.size();
    // usage は作成するバッファの使い道を示すためのもの
    // 今回のように頂点バッファを作る場合、上記のようにvk::BufferUsageFlagBits::eVertexBufferフラグを指定しなければならない
    // 他にも場合によって様々なフラグを指定する必要があり、複数のフラグを指定することもある
    vertexBufferCreateInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst; // eTransferDstを追加
    // sharingModeについては今のところは無視
    // ここではvk::SharingMode::eExclusiveを指定
    vertexBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    
    *result = device.get().createBufferUnique(vertexBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getVertexBufferMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& vertexBuf)
{
    // 実際に使われる頂点バッファのデバイスメモリを確保する

    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements vertexBufMemReq = device.get().getBufferMemoryRequirements(vertexBuf.get());
    
    vk::MemoryAllocateInfo vertexBufMemAllocInfo;
    vertexBufMemAllocInfo.allocationSize = vertexBufMemReq.size;
    
    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (vertexBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal))
        {
            vertexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        LOGERR("Suitable memory type not found. ");
        exit(EXIT_FAILURE);
    }

    LOG("FindVertexMem " << vertexBufMemAllocInfo.memoryTypeIndex);
    
    *result = device.get().allocateMemoryUnique(vertexBufMemAllocInfo);
    // デバイスメモリが確保出来たら bindBufferMemoryで結び付ける
    // 第1引数は結びつけるバッファ、第2引数は結びつけるデバイスメモリ
    // 第3引数は、確保したデバイスメモリのどこを(先頭から何バイト目以降を)使用するかを指定するもの
    device.get().bindBufferMemory(vertexBuf.get(), result->get(), 0);
    return result;
}

std::shared_ptr<vk::UniqueBuffer> getStagingVertexBuffer(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::BufferCreateInfo stagingBufferCreateInfo;
    stagingBufferCreateInfo.size = sizeof(Vertex) * vertices.size();
    stagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    stagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    *result = device.get().createBufferUnique(stagingBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getStagingVertexBufferMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& stagingVertexBuf)
{
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements vertexBufMemReq = device.get().getBufferMemoryRequirements(stagingVertexBuf.get());
    
    vk::MemoryAllocateInfo vertexBufMemAllocInfo;
    vertexBufMemAllocInfo.allocationSize = vertexBufMemReq.size;
    
    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (vertexBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible))
        {
            vertexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        LOGERR("Suitable memory type not found. ");
        exit(EXIT_FAILURE);
    }
    
    LOG("FindVertexMem " << vertexBufMemAllocInfo.memoryTypeIndex);
    
    *result = device.get().allocateMemoryUnique(vertexBufMemAllocInfo);
    device.get().bindBufferMemory(stagingVertexBuf.get(), result->get(), 0);
    return result;
}

void writeStagingVertexBuffer(vk::UniqueDevice& device, vk::UniqueDeviceMemory& stagingVertexBufMem)
{
    // デバイスメモリに書き込むために、メモリマッピングというものをする
    // これは操作したい対象のデバイスメモリを仮想的にアプリケーションのメモリ空間に対応付けることで操作出来るようにするもの
    // 対象のデバイスメモリを直接操作するわけにはいかないのでこういう形になっている
    void* pStagingVertexBufMem = device.get().mapMemory(stagingVertexBufMem.get(), 0, sizeof(Vertex) * vertices.size());

    std::memcpy(pStagingVertexBufMem, vertices.data(), sizeof(Vertex) * vertices.size());

    vk::MappedMemoryRange flushMemoryRange;
    flushMemoryRange.memory = stagingVertexBufMem.get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = sizeof(Vertex) * vertices.size();
    
    // 書き込んだら flushMappedMemoryRangesメソッドを呼ぶことで書き込んだ内容がデバイスメモリに反映される
    // マッピングされたメモリはあくまで仮想的にデバイスメモリと対応付けられているだけ
    // 「同期しておけよ」と念をおさないとデータが同期されない可能性がある
    device.get().flushMappedMemoryRanges({ flushMemoryRange });
    // 作業が終わった後はunmapMemoryできちんと後片付けをする
    device.get().unmapMemory(stagingVertexBufMem.get());
}

void sendVertexBuffer(vk::UniqueDevice& device, uint32_t queueFamilyIndex, vk::Queue& graphicsQueue, vk::UniqueBuffer& stagingBuf, vk::UniqueBuffer& vertexBuf)
{
    // こちらはメモリマッピングではデータを入れられない ホスト可視でないため
    // ホスト可視でないメモリはCPUからは触れない
    // GPUにコピーさせる訳だが、GPUに命令するということは例によってコマンドバッファとキューを使う
    // データの転送用に、コマンドバッファを作成するコードを別途追加する
    vk::CommandPoolCreateInfo tmpCmdPoolCreateInfo;
    tmpCmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
    // vk::CommandPoolCreateFlagBits::eTransientフラグを指定
    // これは比較的すぐに使ってすぐに役目を終えるコマンドバッファ用であることを意味するフラグ
    // 必須ではないが指定しておくと内部的に最適化が起きる可能性がある
    tmpCmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    vk::UniqueCommandPool tmpCmdPool = device->createCommandPoolUnique(tmpCmdPoolCreateInfo);
    
    vk::CommandBufferAllocateInfo tmpCmdBufAllocInfo;
    tmpCmdBufAllocInfo.commandPool = tmpCmdPool.get();
    tmpCmdBufAllocInfo.commandBufferCount = 1;
    tmpCmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> tmpCmdBufs = device->allocateCommandBuffersUnique(tmpCmdBufAllocInfo);

    // バッファ間でデータをコピーするには copyBuffer を使う
    // そしてコピーするバッファやコピーする領域を指定するために vk::BufferCopy 構造体を使う
    // srcOffsetは転送元バッファの先頭から何バイト目を読み込むというデータ位置、dstOffsetは転送先バッファの先頭から何バイト目に書き込むというデータ位置、sizeはデータサイズを表す
    // memcpyの引数と似たような感じだと理解すると分かりやすい
    // あとはキューに投げるだけ
    vk::BufferCopy bufCopy;
    bufCopy.srcOffset = 0;
    bufCopy.dstOffset = 0;
    bufCopy.size = sizeof(Vertex) * vertices.size();

    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    tmpCmdBufs[0]->begin(cmdBeginInfo);
    tmpCmdBufs[0]->copyBuffer(stagingBuf.get(), vertexBuf.get(), {bufCopy});
    tmpCmdBufs[0]->end();

    vk::CommandBuffer submitCmdBuf[1] = {tmpCmdBufs[0].get()};
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = submitCmdBuf;

    graphicsQueue.submit({submitInfo});
    graphicsQueue.waitIdle();
}

std::shared_ptr<std::vector<vk::VertexInputBindingDescription>> getVertexBindingDescription()
{
    std::shared_ptr<std::vector<vk::VertexInputBindingDescription>> result = std::make_shared<std::vector<vk::VertexInputBindingDescription>>();

    // このままではバッファがどういう形式で頂点のデータを持っているのか、Vulkanのシステム側には分からない
    // Vertex構造体というこちらが勝手に定義してみただけの構造体のことは知りようがない
    // このままではシェーダに読み込ませられない
    // そこで頂点入力デスクリプションというものを用意する
    // 適切な説明(デスクリプション)を与えることで「こういう形式でこういうデータが入っている」という情報を与えることができる
    // データだけでなく説明を与えて初めてシェーダからデータを読み込める

    // 頂点入力デスクリプションには2種類ある
    // アトリビュートデスクリプション(vk::VertexInputAttributeDescription)
    // バインディングデスクリプション(vk::VertexInputBindingDescription)
    // これはひとえに、シェーダに与えるデータを分ける単位が「バインディング」と「アトリビュート」の2段階あるため

    // バインディングが一番上の単位
    // 基本的に1つのバインディングに対して1つのバッファ(の範囲)が結び付けられる
    // あるバインディングに含まれるデータは一定の大きさごとに切り分けられ、各頂点のデータとして解釈される
    // アトリビュートはより細かい単位になる
    // 1つの頂点のデータは、1つまたは複数のアトリビュートとして分けられる
    // 一つの頂点データは「座標」とか「色」みたいな複数のデータを内包していますが、この1つ1つのデータが1つのアトリビュートに相当することになる
    // (注意点として、1つのアトリビュートは「多次元ベクトル」や「行列」だったりできるので、アトリビュートは「x座標」とか「y座標」みたいな単位よりはもう少し大きい単位になる)

    // バインディングデスクリプションは、1つのバインディングのデータをどうやって各頂点のデータに分割するかという情報を設定する。
    // 具体的には、1つの頂点データのバイト数などを設定する。
    // そしてアトリビュートデスクリプションは、1つの頂点データのどの位置からどういう形式でアトリビュートのデータを取り出すかという情報を設定する。
    // アトリビュートデスクリプションはアトリビュートの数だけ作成する。
    // 1つの頂点データが3種類のデータを含んでいるならば、3つのアトリビュートデスクリプションを作らなければならない。
    // 今回のデータだと含んでいるデータは「座標」だけなので1つだけになるが、複数のアトリビュートを含む場合はその数だけ作成する。

    // bindingは説明の対象となるバインディングの番号である。各頂点入力バインディングには0から始まる番号が振ってある。ここでは0番を使うことにする。
    // strideは1つのデータを取り出す際の刻み幅である。つまり上の図でいう所の「1つの頂点データ」の大きさである。ここではVertex構造体のバイト数を指定している。
    // 前節までで用意したデータはVertex構造体が並んでいるわけなので、各データを取り出すには当然Vertex構造体のバイト数ずつずらして読み取ることになる。
    // inputRateにはvk::VertexInputRate::eVertexを指定する。1頂点ごとにデータを読み込む、というだけの意味である。インスタンス化というものを行う場合に別の値を指定する。
    vk::VertexInputBindingDescription vertexBindingDescription;
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = sizeof(Vertex);
    vertexBindingDescription.inputRate = vk::VertexInputRate::eVertex;

    (*result).push_back(vertexBindingDescription);
    return result;
}

std::shared_ptr<std::vector<vk::VertexInputAttributeDescription>> getVertexInputDescription()
{
    std::shared_ptr<std::vector<vk::VertexInputAttributeDescription>> result = std::make_shared<std::vector<vk::VertexInputAttributeDescription>>();

    // アトリビュートの読み込み方の説明情報
    // 今度は vk::VertexInputAttributeDescription を用意する
    // 今回用意した頂点データが含む情報は「座標」だけなので、用意するアトリビュートデスクリプションは1つだけ
    //
    // bindingはデータの読み込み元のバインディングの番号を指定。上で0番のバインディングを使うことにしたので、ここでも0にする。
    // locationはシェーダにデータを渡す際のデータ位置。
    //
    // 頂点データの用意に書いたシェーダのコードを見てみる
    // layout(location = 0) in vec2 inPos;
    // location = 0という部分がある。シェーダがアトリビュートのデータを受け取る際はこのようにデータ位置を指定する。
    // このlayout(location = xx)の指定とアトリビュートデスクリプションのlocationの位置は対応付けて書かなければならない。
    // locationの対応付けによって、シェーダで読み込む変数とVulkanが用意したアトリビュートの対応が付くのである。
    //
    // formatはデータ形式である。今回渡すのは32bitのfloat型が2つある2次元ベクトルなので、それを表すeR32G32Sfloatを指定する。
    // ここで使われているvk::Formatは本来ピクセルの色データのフォーマットを表すものなのでRとかGとか入っているが、それらはここでは無視してほしい。
    // ここでは 「32bit, 32bit, それぞれSigned(符号付)floatである」という意味を表すためだけにこれが指定されている。
    // ちなみにfloat型の3次元ベクトルを渡す際にはeR32G32B32Sfloatとか指定する。ここでもRGBの文字に深い意味はない。
    // 色のデータを渡すにしろ座標データを渡すにしろこういう名前の値を指定する。違和感があるかもしれないが、こういうものなので仕方ない。
    // offsetは頂点データのどの位置からデータを取り出すかを示す値。今回は1つしかアトリビュートが無いので0を指定しているが、複数のアトリビュートがある場合にはとても重要なものである。
    (*result).push_back(vk::VertexInputAttributeDescription());
    (*result).push_back(vk::VertexInputAttributeDescription());
    (*result).push_back(vk::VertexInputAttributeDescription());

    (*result)[0].binding = 0;
    (*result)[0].location = 0;
    (*result)[0].format = vk::Format::eR32G32B32Sfloat;
    (*result)[0].offset = offsetof(Vertex, pos);
    
    (*result)[1].binding = 0;
    (*result)[1].location = 1;
    (*result)[1].format = vk::Format::eR32G32B32Sfloat;
    (*result)[1].offset = offsetof(Vertex, color);

    // イメージのuvを追加
    (*result)[2].binding = 0;
    (*result)[2].location = 2;
    (*result)[2].format = vk::Format::eR32G32Sfloat;
    (*result)[2].offset = offsetof(Vertex, texUV);

    return result;
}

std::shared_ptr<vk::UniqueBuffer> getIndexBuffer(vk::UniqueDevice& device)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();

    vk::BufferCreateInfo indexBufferCreateInfo;
    indexBufferCreateInfo.size = sizeof(uint16_t) * indices.size();
    indexBufferCreateInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst; 
    indexBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    
    *result = device.get().createBufferUnique(indexBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getIndexBufferMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& indexBuf)
{
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements indexBufMemReq = device.get().getBufferMemoryRequirements(indexBuf.get());
    
    vk::MemoryAllocateInfo indexBufMemAllocInfo;
    indexBufMemAllocInfo.allocationSize = indexBufMemReq.size;
    
    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (indexBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal))
        {
            indexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        LOGERR("Suitable memory type not found. ");
        exit(EXIT_FAILURE);
    }
    
    *result = device.get().allocateMemoryUnique(indexBufMemAllocInfo);
    device.get().bindBufferMemory(indexBuf.get(), result->get(), 0);
    return result;
}

std::shared_ptr<vk::UniqueBuffer> getStagingIndexBuffer(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::BufferCreateInfo stagingBufferCreateInfo;
    stagingBufferCreateInfo.size = sizeof(uint16_t) * indices.size();
    stagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    stagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    *result = device.get().createBufferUnique(stagingBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getStagingIndexBufferMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& stagingIndexBuf)
{
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements indexBufMemReq = device.get().getBufferMemoryRequirements(stagingIndexBuf.get());
    
    vk::MemoryAllocateInfo indexBufMemAllocInfo;
    indexBufMemAllocInfo.allocationSize = indexBufMemReq.size;
    
    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (indexBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible))
        {
            indexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        LOGERR("Suitable memory type not found. ");
        exit(EXIT_FAILURE);
    }
    
    *result = device.get().allocateMemoryUnique(indexBufMemAllocInfo);
    device.get().bindBufferMemory(stagingIndexBuf.get(), result->get(), 0);
    return result;
}

void writeStagingIndexBuffer(vk::UniqueDevice& device, vk::UniqueDeviceMemory& stagingIndexBufMem)
{
    void* pStagingIndexBufMem = device.get().mapMemory(stagingIndexBufMem.get(), 0, sizeof(uint16_t) * indices.size());

    std::memcpy(pStagingIndexBufMem, indices.data(), sizeof(uint16_t) * indices.size());

    vk::MappedMemoryRange flushMemoryRange;
    flushMemoryRange.memory = stagingIndexBufMem.get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = sizeof(uint16_t) * indices.size();
    
    device.get().flushMappedMemoryRanges({ flushMemoryRange });
    device.get().unmapMemory(stagingIndexBufMem.get());
}

void sendIndexBuffer(vk::UniqueDevice& device, uint32_t queueFamilyIndex, vk::Queue& graphicsQueue, vk::UniqueBuffer& stagingBuf, vk::UniqueBuffer& indexBuf)
{
    vk::CommandPoolCreateInfo tmpCmdPoolCreateInfo;
    tmpCmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
    tmpCmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    vk::UniqueCommandPool tmpCmdPool = device->createCommandPoolUnique(tmpCmdPoolCreateInfo);
    
    vk::CommandBufferAllocateInfo tmpCmdBufAllocInfo;
    tmpCmdBufAllocInfo.commandPool = tmpCmdPool.get();
    tmpCmdBufAllocInfo.commandBufferCount = 1;
    tmpCmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> tmpCmdBufs = device->allocateCommandBuffersUnique(tmpCmdBufAllocInfo);

    vk::BufferCopy bufCopy;
    bufCopy.srcOffset = 0;
    bufCopy.dstOffset = 0;
    bufCopy.size = sizeof(uint16_t) * indices.size();

    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    tmpCmdBufs[0]->begin(cmdBeginInfo);
    tmpCmdBufs[0]->copyBuffer(stagingBuf.get(), indexBuf.get(), {bufCopy});
    tmpCmdBufs[0]->end();

    vk::CommandBuffer submitCmdBuf[1] = {tmpCmdBufs[0].get()};
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = submitCmdBuf;

    graphicsQueue.submit({submitInfo});
    graphicsQueue.waitIdle();
}

std::shared_ptr<vk::UniqueBuffer> getUniformBuffer(vk::UniqueDevice& device)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();
    
    vk::BufferCreateInfo uniformBufferCreateInfo;
    uniformBufferCreateInfo.size = sizeof(SceneData);
    // usageはvk::BufferUsageFlagBits::eUniformBufferを指定
    uniformBufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
    uniformBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    *result = device.get().createBufferUnique(uniformBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getUniformBufferMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& uniformBuf)
{    
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();
    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements uniformBufMemReq = device->getBufferMemoryRequirements(uniformBuf.get());

    vk::MemoryAllocateInfo uniformBufMemAllocInfo;
    uniformBufMemAllocInfo.allocationSize = uniformBufMemReq.size;

    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (uniformBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)) 
        {
            uniformBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        LOGERR("Suitable memory type not found. ");
        exit(EXIT_FAILURE);
    }

    *result = device.get().allocateMemoryUnique(uniformBufMemAllocInfo);
    device->bindBufferMemory(uniformBuf.get(), result->get(), 0);
    return result;
}

void* mapUniformBuffer(vk::UniqueDevice& device, vk::UniqueDeviceMemory& uniformBufMem)
{
    return device.get().mapMemory(uniformBufMem.get(), 0, sizeof(SceneData));
}

void writeUniformBuffer(void* pUniformBufMem, vk::UniqueDevice& device, vk::UniqueDeviceMemory& uniformBufMem, uint32_t screenWidth, uint32_t screenHeight, int deltaTime)
{
    static float time = 0;

    time += deltaTime / 1000.0f;

    float rotation = (time / 10) * 2 * 3.14f;

    Mat4x4 model1 = translationMatrix({cos(rotation), sin(rotation), 0.0f}) * rotationMatrix({0.0f, 0.0f, 1.0f}, rotation) * scaleMatrix(1.0f);
    Mat4x4 model2 = translationMatrix({-cos(rotation), -sin(rotation), 0.0f}) * rotationMatrix({0.0f, 0.0f, 1.0f}, rotation) * scaleMatrix(1.0f);

    Mat4x4 view = viewMatrix({0.0f, 2.0f, 1.5f}, {0.0f, -0.707f, -0.707f}, {0.0f, -0.707f, +0.707f});
    Mat4x4 proj = projectionMatrix(3.14f / 3, float(screenHeight) / float(screenWidth), 0.1f, 100.0f);

    sceneData.mvpMatrix[0] = proj * view * model1;
    sceneData.mvpMatrix[1] = proj * view * model2;

    // sceneData.rectCenter = Vec2{ 0.3f * std::cos(time), 0.3f * std::sin(time) };

    std::memcpy(pUniformBufMem, &sceneData, sizeof(SceneData));

    vk::MappedMemoryRange flushMemoryRange;
    flushMemoryRange.memory = uniformBufMem.get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = sizeof(SceneData);

    device.get().flushMappedMemoryRanges({ flushMemoryRange });
    // device.get().unmapMemory(uniformBufMem.get());
}

void unmapUniformBuffer(vk::UniqueDevice& device, vk::UniqueDeviceMemory& uniformBufMem)
{
    device.get().unmapMemory(uniformBufMem.get());
}

std::shared_ptr<std::vector<vk::UniqueDescriptorSetLayout>> getDiscriptorSetLayouts(vk::UniqueDevice& device)
{
    std::shared_ptr<std::vector<vk::UniqueDescriptorSetLayout>> result = std::make_shared<std::vector<vk::UniqueDescriptorSetLayout>>();

    // vk::DescriptorSetLayoutBindingがデスクリプタ1つの情報を表す
    // これの配列からデスクリプタセットレイアウトを作成
    //
    // binding はシェーダと結びつけるための番号を表す = バインディング番号
    //    頂点シェーダにlayout(set = 0, binding = 0)などと指定したが、このときのbindingの数字と揃えてあることに注意
    // descriptorType はデスクリプタの種別を示す
    //    今回シェーダに渡すものはバッファなのでvk::DescriptorType::eUniformBufferを指定
    // descriptorCount はデスクリプタの個数を表す
    //    デスクリプタは配列として複数のデータを持てるが、ここにその要素数を指定する 今回は1個だけなので1を指定
    // stageFlags はデータを渡す対象となるシェーダを示す 
    //    今回は頂点シェーダだけに渡すのでvk::ShaderStageFlagBits::eVertexを指定 フラグメントシェーダに渡したい場合はvk::ShaderStageFlagBits::eFragmentを指定します。ビットマスクなので、ORで重ねれば両方に渡すことも可能です。

    vk::DescriptorSetLayoutBinding descSetLayoutBinding[2];
    descSetLayoutBinding[0].binding = 0;
    descSetLayoutBinding[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    descSetLayoutBinding[0].descriptorCount = 1;
    descSetLayoutBinding[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
    // 画像のサンプラーを追加
    descSetLayoutBinding[1].binding = 1;
    descSetLayoutBinding[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descSetLayoutBinding[1].descriptorCount = 1;
    descSetLayoutBinding[1].stageFlags = vk::ShaderStageFlagBits::eFragment;
    
    vk::DescriptorSetLayoutCreateInfo descSetLayoutCreateInfo{};
    descSetLayoutCreateInfo.bindingCount = std::size(descSetLayoutBinding);
    descSetLayoutCreateInfo.pBindings = descSetLayoutBinding;
    
    // デスクリプタセットレイアウトを作成したあとはそれをパイプラインレイアウトに設定する必要がある
    // パイプラインは描画の手順を表すオブジェクト
    // 頂点入力デスクリプションなどと同様、シェーダへのデータの読み込ませ方はここで設定する
    (*result).push_back(device->createDescriptorSetLayoutUnique(descSetLayoutCreateInfo));
    return result;
}

std::shared_ptr<vk::UniqueDescriptorPool> getDescriptorPool(vk::UniqueDevice& device)
{
    std::shared_ptr<vk::UniqueDescriptorPool> result = std::make_shared<vk::UniqueDescriptorPool>();

    // 次にデスクリプタプールを作成
    // 重要なこととしてデスクリプタには種類がある
    // そのためデスクリプタプールも、「この種類のデスクリプタをこの数」と指定して作成する必要がある
    // 必要な種類と数をきちんと指定する
    vk::DescriptorPoolSize descPoolSize[2];
    descPoolSize[0].type = vk::DescriptorType::eUniformBuffer;
    descPoolSize[0].descriptorCount = 1;
    // 画像のサンプラーを追加
    // eCombinedImageSamplerというタイプのデスクリプタを使用
    // これはイメージとサンプラーを束ねた情報のデスクリプタ
    // シェーダからテクスチャを利用できるようにする場合、イメージとサンプラーをセットで渡すのが一般的
    descPoolSize[1].type = vk::DescriptorType::eCombinedImageSampler;
    descPoolSize[1].descriptorCount = 1;

    // vk::DescriptorPoolSizeの配列を poolSizeCountとpPoolSizes に指定
    // vk::DescriptorPoolSizeはtypeがデスクリプタの種類でdescriptorCountがデスクリプタの数
    // vk::DescriptorPoolCreateInfoに maxSets というメンバがあるが、これはデスクリプタプールから作成するデスクリプタセットの数の上限を指定
    vk::DescriptorPoolCreateInfo descPoolCreateInfo;
    descPoolCreateInfo.poolSizeCount = std::size(descPoolSize);
    descPoolCreateInfo.pPoolSizes = descPoolSize;
    descPoolCreateInfo.maxSets = 1;

    *result = device->createDescriptorPoolUnique(descPoolCreateInfo);
    return result;
}

std::shared_ptr<std::vector<vk::UniqueDescriptorSet>> getDescprotorSets(vk::UniqueDevice& device, vk::UniqueDescriptorPool& descPool, std::vector<vk::DescriptorSetLayout>& descSetLayouts)
{
    std::shared_ptr<std::vector<vk::UniqueDescriptorSet>> result = std::make_shared<std::vector<vk::UniqueDescriptorSet>>();

    // 最後にデスクリプタセットを作成
    // デスクリプタセットはデバイスの allocateDescriptorSetsメソッドを用いて作成する
    // createではなくallocateなあたりで「デスクリプタプールから割り当てる」という気持ちが読み取れる

    // 構造を示すデスクリプタセットレイアウト、実体を記録するデスクリプタプールを指定して作成 
    // またSetsと複数形になっているところから分かる通り、一度に複数のデスクリプタセットを作成できる
    // デスクリプタセットレイアウトが配列によって複数渡せるようになっているのはそのため

    // デスクリプタ関係でややこしいのは、デスクリプタセットを表すvk::DescriptorSet、デスクリプタプールを表すvk::DescriptorPoolなどは存在するのに、
    // 1つのデスクリプタを表すvk::Descriptorなどというオブジェクトは存在しないということ
    // デスクリプタは常に「あるデスクリプタセットの何番目のデスクリプタ」という形でしか触ることができない
    
    vk::DescriptorSetAllocateInfo descSetAllocInfo;
    
    descSetAllocInfo.descriptorPool = descPool.get();
    descSetAllocInfo.descriptorSetCount = descSetLayouts.size();
    descSetAllocInfo.pSetLayouts = descSetLayouts.data();
    
    *result = device->allocateDescriptorSetsUnique(descSetAllocInfo);
    return result;
}

void writeDescriptorSets(vk::UniqueDevice& device, std::vector<vk::UniqueDescriptorSet>& descSets, vk::UniqueBuffer& uniformBuf, vk::UniqueImageView& texImageView, vk::UniqueSampler& texSampler)
{
    // 今はまだデスクリプタセットを作っただけでその中身は何もないので、updateDescriptorSetsで中身を設定する必要がある
    // デスクリプタへの書き込み情報はvk::WriteDescriptorSet構造体で表される

    // 一度に複数のvk::WriteDescriptorSetをupdateDescriptorSetsに渡せば複数のデスクリプタの更新を行うことも可能
    // dstSet は書き込みの対象とするデスクリプタセット

    // dstBinding は書き込みの対象とするデスクリプタのバインディング番号
    // dstArrayElement は書き込みの対象とするデスクリプタの配列要素の番号
    // デスクリプタは配列であって複数のデータが持てる、という話をしたが、その配列上の何番からの要素に書き込むかをここに指定する
    
    // descriptorType は書き込みの対象となるデスクリプタの種別
    // ここではvk::DescriptorType::eUniformBufferを指定

    // バッファタイプのデスクリプタに書き込む場合、vk::DescriptorBufferInfo の配列を pBufferInfoとdescriptorCount に指定
    // vk::DescriptorBufferInfoのメンバについて解説すると、buffer が用いるバッファ、offset がバッファ上の先頭から何バイト目からをデータとして用いるか、range がデータの大きさ(バイト数)になる

    // 混乱を避けるために書いておくと、デスクリプタが持っている情報は「0.3, -0.2」といった数値ではなく、あくまで「このバッファのこの位置のデータをシェーダに渡す」という情報
    // デスクリプタ=ポインタみたいなものと考えた方が良い
    // 従って、データの変更のたびにupdateDescriptorSetsを呼ばなくてもバッファのメモリ内容を書き換えればシェーダに渡すデータも変えられる
    // 途中で別のバッファや別の領域を使うといったことをしない限り、updateDescriptorSetsによる設定は最初の1回だけで十分です。

    vk::WriteDescriptorSet writeDescSet;
    writeDescSet.dstSet = descSets[0].get();
    writeDescSet.dstBinding = 0;
    writeDescSet.dstArrayElement = 0;
    writeDescSet.descriptorType = vk::DescriptorType::eUniformBuffer;
    
    vk::DescriptorBufferInfo descBufInfo[1];
    descBufInfo[0].buffer = uniformBuf.get();
    descBufInfo[0].offset = 0;
    descBufInfo[0].range = sizeof(SceneData);
    
    writeDescSet.descriptorCount = 1;
    writeDescSet.pBufferInfo = descBufInfo;
    
    device->updateDescriptorSets({ writeDescSet }, {});

    // ユニフォームバッファの時はvk::DescriptorBufferInfo構造体を使ったが、画像サンプラを設定する場合はvk::DescriptorImageInfo構造体を使用
    // imegeViewにイメージビューを設定
    // imageLayoutにイメージのレイアウトをeShaderReadOnlyOptimalに設定
    // sampler にサンプラーを設定します。
    // 作成したvk::DescriptorImageInfo構造体へのポインタは、vk::WriteDescriptorSetの pImageInfo に設定します。

    vk::WriteDescriptorSet writeTexDescSet;
    writeTexDescSet.dstSet = descSets[0].get();
    writeTexDescSet.dstBinding = 1;
    writeTexDescSet.dstArrayElement = 0;
    writeTexDescSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;

    vk::DescriptorImageInfo descImgInfo[1];
    descImgInfo[0].imageView = texImageView.get();
    descImgInfo[0].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    descImgInfo[0].sampler = texSampler.get();

    writeTexDescSet.descriptorCount = std::size(descImgInfo);
    writeTexDescSet.pImageInfo = descImgInfo;

    device->updateDescriptorSets({ writeTexDescSet }, {});
}

std::shared_ptr<std::vector<vk::PushConstantRange>> getPushConstantRanges()
{
    // 小さい数値データはプッシュ定数、大きなバッファやテクスチャ画像などのデータはデスクリプタを使う
    // プッシュ定数は128バイトまでしか容量を保証されていない(最低保証)
    std::shared_ptr<std::vector<vk::PushConstantRange>> result = std::make_shared<std::vector<vk::PushConstantRange>>();

    vk::PushConstantRange pushConstantRange;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ObjectData);
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;

    (*result).push_back(pushConstantRange);
    return result;
}

void writePushConstant(int id)
{
    objectData.id = id;
}




std::shared_ptr<vk::UniqueImage> getImage(vk::UniqueDevice& device, int imgWidth, int imgHeight, int imgCh)
{
    std::shared_ptr<vk::UniqueImage> result = std::make_shared<vk::UniqueImage>();

    vk::ImageCreateInfo texImgCreateInfo;
    texImgCreateInfo.imageType = vk::ImageType::e2D;
    texImgCreateInfo.extent = vk::Extent3D(imgWidth, imgHeight, 1);
    texImgCreateInfo.mipLevels = 1;
    texImgCreateInfo.arrayLayers = 1;
    texImgCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
    texImgCreateInfo.tiling = vk::ImageTiling::eOptimal;
    texImgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    // vk::ImageUsageFlagBits::eSampledフラグを立ている
    // これはイメージをテクスチャサンプリングに使うことを示している
    // vk::ImageUsageFlagBits::eTransferDstが指定してあるのは、後でステージングバッファからデータを転送するから
    texImgCreateInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    texImgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    texImgCreateInfo.samples = vk::SampleCountFlagBits::e1;
    
    *result = device->createImageUnique(texImgCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getImageMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueImage& texImage)
{
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();
    vk::MemoryRequirements imgBufMemReq = device->getImageMemoryRequirements(texImage.get());

    vk::MemoryAllocateInfo imgBufMemAllocInfo;
    imgBufMemAllocInfo.allocationSize = imgBufMemReq.size;

    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (imgBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal))
        {
            imgBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound)
    {
        LOGERR("Suitable memory type not found for image.");
        exit(EXIT_FAILURE);
    }

    *result = device.get().allocateMemoryUnique(imgBufMemAllocInfo);
    device.get().bindImageMemory(texImage.get(), result->get(), 0);
    return result;
}

std::shared_ptr<vk::UniqueBuffer> getStagingImageBuffer(vk::UniqueDevice& device, int imgWidth, int imgHeight, int imgCh)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();
    size_t imgDataSize = imgCh * imgWidth * imgHeight;

    vk::BufferCreateInfo imgStagingBufferCreateInfo;
    imgStagingBufferCreateInfo.size = imgDataSize;
    imgStagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    imgStagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    *result = device->createBufferUnique(imgStagingBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getStagingImageMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& imgStagingBuf)
{
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();
    vk::MemoryRequirements imgStagingBufMemReq = device->getBufferMemoryRequirements(imgStagingBuf.get());

    vk::MemoryAllocateInfo imgStagingBufMemAllocInfo;
    imgStagingBufMemAllocInfo.allocationSize = imgStagingBufMemReq.size;

    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
    {
        if (imgStagingBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible))
        {
            imgStagingBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound)
    {
        LOGERR("Suitable memory type not found for image.");
        exit(EXIT_FAILURE);
    }

    *result = device.get().allocateMemoryUnique(imgStagingBufMemAllocInfo);
    device.get().bindBufferMemory(imgStagingBuf.get(), result->get(), 0);
    return result;
}

void writeImageBuffer(vk::UniqueDevice& device, vk::UniqueDeviceMemory& imgStagingBufMemory, void* pImgData, int imgWidth, int imgHeight, int imgCh)
{
    size_t imgDataSize = imgCh * imgWidth * imgHeight;
    void *pImgStagingBufMem = device->mapMemory(imgStagingBufMemory.get(), 0, imgDataSize);

    std::memcpy(pImgStagingBufMem, pImgData, imgDataSize);

    vk::MappedMemoryRange flushMemoryRange;
    flushMemoryRange.memory = imgStagingBufMemory.get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = imgDataSize;

    device->flushMappedMemoryRanges({flushMemoryRange});

    device->unmapMemory(imgStagingBufMemory.get());
}

void sendImageBuffer(vk::UniqueDevice& device, uint32_t queueFamilyIndex, vk::Queue& graphicsQueue, vk::UniqueBuffer& imgStagingBuf, vk::UniqueImage& texImage, int imgWidth, int imgHeight)
{
    vk::CommandPoolCreateInfo tmpCmdPoolCreateInfo;
    tmpCmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
    tmpCmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    vk::UniqueCommandPool tmpCmdPool = device->createCommandPoolUnique(tmpCmdPoolCreateInfo);

    vk::CommandBufferAllocateInfo tmpCmdBufAllocInfo;
    tmpCmdBufAllocInfo.commandPool = tmpCmdPool.get();
    tmpCmdBufAllocInfo.commandBufferCount = 1;
    tmpCmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> tmpCmdBufs = device->allocateCommandBuffersUnique(tmpCmdBufAllocInfo);

    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    tmpCmdBufs[0]->begin(cmdBeginInfo);
    // 以前にバッファからバッファへデータをコピーしたときはvk::CommandBuffer::copyBuffer()を使用した
    // 今回はバッファからイメージにデータをコピーするので、vk::CommandBuffer::copyBufferToImage()という別のコマンドを使用する
    
    // データをコピーするためにイメージレイアウトを適切に変換する
    // 1. texImageのレイアウトをeTransferDstOptimalに変換
    //     eTransferDstOptimalはデータのコピー先となるためのレイアウト
    // 2. copyBufferToImage()でデータをtexImageにコピー
    // 3. texImageのレイアウトをeShaderReadOnlyOptimalに変換 
    //     eShaderReadOnlyOptimalは、シェーダからの読み込みアクセスのためのレイアウト
    //     今回のようにテクスチャとして扱う場合にはこれを指定するのが最適

    // 今回はパイプラインバリアを用いる
    // 本来パイプラインバリアはフェンスやセマフォのように同期処理のための道具
    // だがイメージのレイアウト変換という機能も副次的に付いているため、これを使う
    // (レンダーパスも、主目的としてはGPUにおける処理の依存関係を表すものなのに、イメージレイアウトの変換処理も行っていた
    // ある処理の中ではイメージをこう扱ってこちらの処理ではイメージをこう利用する...といったケースを考えると、処理の切れ目に変換が入るのは自然なのかも知れない

    // 1
    {
        // vk::ImageMemoryBarrier構造体でイメージのメモリ保護および変換に関する情報を設定し、pipelineBarrier()の引数に渡す
        vk::ImageMemoryBarrier barrior;
        // oldLayoutとnewLayoutがレイアウトの変換を示している
        // createImage()のところで指定しているように、イメージ作成時点ではeUndefinedなのでoldLayoutにはeUndefinedを指定
        // なお、createImage()の時点でeTransferDstOptimalを指定することはできない
        // createImage()で初期状態として指定できるのはeUndefinedもしくはePreinitialized
        barrior.oldLayout = vk::ImageLayout::eUndefined;
        barrior.newLayout = vk::ImageLayout::eTransferDstOptimal;
        barrior.image = texImage.get();
        // 他の引数はイメージのレイアウト変換ではなく、「同期処理」というパイプラインバリア本来の機能のための様々な指定
        // srcQueueFamilyIndex / dstQueueFamilyIndexは、バリアの前と後で別のキューからイメージを扱う場合に使うも
        // 今回は使わないのでVK_QUEUE_FAMILY_IGNOREDを指定
        barrior.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrior.subresourceRange.baseMipLevel = 0;
        barrior.subresourceRange.levelCount = 1;
        barrior.subresourceRange.baseArrayLayer = 0;
        barrior.subresourceRange.layerCount = 1;
        // srcAccessMask / dstAccessMaskにはそれぞれパイプラインバリアの前と後で対象のリソースに行うアクセス処理を示す
        // これで処理の依存関係を示すことができる
        // srcAccessMaskに指定した処理が終わるまで、dstAccessMaskに指定した処理は行われない
        // ここでは終わるまで待つ必要のある処理は存在しないのでsrcAccessMaskは0
        // 画像データのコピー処理はレイアウト変換が終わってから行われないと困るので、dstAccessMaskにeTransferWriteを指定する
        barrior.srcAccessMask = vk::AccessFlagBits::eNone;
        barrior.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        // pipelineBarrierの第1,第2引数にはそれぞれパイプラインバリアの前と後で待たれるパイプラインステージを指定
        // ここでも処理の依存関係を示すことができる
        // 第1引数(バリアの前側)にeTopOfPipeを指定しているが、これは何も待たれないという意味
        // 第2引数(バリアの後側)にeTransferを指定していますが、こちらはデータ転送処理の意味
        //     データ転送はグラフィックスパイプラインのステージではないが、ここでは処理段階の一種としてこのように指定する
        tmpCmdBufs[0]->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, {barrior});
    }

    // 2
    {
        // コピー処理
        vk::BufferImageCopy imgCopyRegion;
        // bufferOffsetは、「バッファの何バイト目からのデータを使う」という情報を示す
        imgCopyRegion.bufferOffset = 0;
        imgCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgCopyRegion.imageSubresource.mipLevel = 0;
        imgCopyRegion.imageSubresource.baseArrayLayer = 0;
        imgCopyRegion.imageSubresource.layerCount = 1;
        // imageSubresource, imageOffset, imageExtentは画像のコピー先の位置を示すもの
        // 見た通りなので詳細な説明は割愛
        imgCopyRegion.imageOffset = vk::Offset3D{0, 0, 0};
        imgCopyRegion.imageExtent = vk::Extent3D{uint32_t(imgWidth), uint32_t(imgHeight), 1};
        
        // bufferRowLength, bufferImageHeightは「バッファ上における」イメージの横・縦ピクセル数を示す
        // 例えば転送先の大きさは100x100だけど、バッファ上には200x200の画像データがあるというケースも可能
        // この場合はバッファ上のイメージの一部が切り取られてコピーされる
        // 0を指定した場合は自動的にimageExtentと同じサイズという扱いになる
        imgCopyRegion.bufferRowLength = 0;
        imgCopyRegion.bufferImageHeight = 0;
        
        // copyBufferToImage()の第1引数がコピー元のバッファ、第2引数がコピー先のイメージ、第3引数がコピー先のイメージのレイアウト
        // 第4引数にはvk:BufferImageCopy構造体を指定する
        //     これは複数指定でき、いくつもの領域を同時にコピーできるようになっている
        tmpCmdBufs[0]->copyBufferToImage(imgStagingBuf.get(), texImage.get(), vk::ImageLayout::eTransferDstOptimal, { imgCopyRegion });
    }

    // 3
    {
        vk::ImageMemoryBarrier barrior;
        barrior.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrior.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrior.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.image = texImage.get();
        barrior.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrior.subresourceRange.baseMipLevel = 0;
        barrior.subresourceRange.levelCount = 1;
        barrior.subresourceRange.baseArrayLayer = 0;
        barrior.subresourceRange.layerCount = 1;
        // 同期処理のパラメータとしてsrcAccessMask、dstAccessMask、pipelineBarrier()の第1,第2引数も変えている
        // データのコピーが終わるまでシェーダからアクセスするわけには行かない
        // dstAccessMaskにeShaderReadを指定し、pipelineBarrier()の第2引数にはeFragmentShaderを指定
        // ここではwaitIdleで丁寧に待っているので意味が薄いが、無駄な待ちを入れずどんどんコマンドを飛ばしていくのであれば意味が出テクス
        barrior.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrior.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        tmpCmdBufs[0]->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, {barrior});
    }

    tmpCmdBufs[0]->end();

    vk::CommandBuffer submitCmdBuf[1] = {tmpCmdBufs[0].get()};
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = submitCmdBuf;

    graphicsQueue.submit({submitInfo});
    graphicsQueue.waitIdle();
}

std::shared_ptr<vk::UniqueSampler> getSampler(vk::UniqueDevice& device)
{
    std::shared_ptr<vk::UniqueSampler> result = std::make_shared<vk::UniqueSampler>();

    vk::SamplerCreateInfo samplerCreateInfo;
    // 補間用アルゴリズムを指定する
    // ニアレストネイバー法はvk::Filter::eNearest
    // 線型補間法はvk::Filter::eLinear
    // 2つあるのはそれぞれ、画像の拡大時(magnification)・縮小時(minification)に対応
    samplerCreateInfo.magFilter = vk::Filter::eLinear;
    samplerCreateInfo.minFilter = vk::Filter::eLinear;
    // addressModeU, addressModeV, addressModeWはそれぞれ、X/Y/Z軸においてテクスチャの範囲外の座標にアクセスした場合の挙動を示すもの
    // vk::SamplerAddressMode::eRepeatを指定した場合、ドラゴンクエストのマップのように範囲外にアクセスすると反対側の端に戻って繰り返す
    samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.anisotropyEnable = false;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    samplerCreateInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerCreateInfo.unnormalizedCoordinates = false;
    samplerCreateInfo.compareEnable = false;
    samplerCreateInfo.compareOp = vk::CompareOp::eAlways;
    samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 0.0f;

    *result = device->createSamplerUnique(samplerCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueImageView> getImageView(vk::UniqueDevice& device, vk::UniqueImage& texImage)
{
    std::shared_ptr<vk::UniqueImageView> result = std::make_shared<vk::UniqueImageView>();

    vk::ImageViewCreateInfo texImgViewCreateInfo;
    texImgViewCreateInfo.image = texImage.get();
    texImgViewCreateInfo.viewType = vk::ImageViewType::e2D;
    texImgViewCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
    texImgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    texImgViewCreateInfo.subresourceRange.baseMipLevel = 0;
    texImgViewCreateInfo.subresourceRange.levelCount = 1;
    texImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    texImgViewCreateInfo.subresourceRange.layerCount = 1;

    *result = device->createImageViewUnique(texImgViewCreateInfo);
    return result;
}