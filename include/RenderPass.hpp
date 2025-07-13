#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

// vk::AttachmentDescription は、Vulkan におけるレンダーパスで使用されるアタッチメントの特性を記述するための構造体である。
// レンダーパスは、一連のレンダリング操作を定義するものであり、その中で使用されるカラーバッファ、デプス/ステンシルバッファといったアタッチメントが、
// どのように利用され、どのような特性を持つかを vk::AttachmentDescription を用いて明示的に指定する必要がある。
std::shared_ptr<std::vector<vk::AttachmentDescription>> getAttachmentDescriptions(vk::SurfaceFormatKHR &surfaceFormat)
{
    std::shared_ptr<std::vector<vk::AttachmentDescription>> result = std::make_shared<std::vector<vk::AttachmentDescription>>();
    (*result).push_back(vk::AttachmentDescription());
    (*result).push_back(vk::AttachmentDescription());

    // flags: アタッチメントに関する追加のフラグを指定する。例えば、vk::AttachmentDescriptionFlagBits::eMayAlias は、異なるレンダーパスにおいて同じメモリ領域がエイリアスされる可能性があることを示す。 
    // (*result)[0].flags = vk::AttachmentDescriptionFlagBits::eMayAlias;

    // formatにはイメージのフォーマット情報を指定する必要がある
    // format: アタッチメントが使用するピクセルフォーマットを指定する。
    // これは、カラーバッファであれば vk::Format::eR8G8B8A8Unorm のようなカラーフォーマット、デプス/ステンシルバッファであれば vk::Format::eD32Sfloat のようなデプス/ステンシルフォーマットとなる。
    (*result)[0].format = surfaceFormat.format;

    // samples: アタッチメントが使用するサンプリング数を指定する。
    // マルチサンプリングを行う場合は、vk::SampleCountFlagBits::e4 のように複数のサンプル数を指定する。単一サンプリングの場合は vk::SampleCountFlagBits::e1 を指定する。
    (*result)[0].samples = vk::SampleCountFlagBits::e1;

    // loadOp: レンダーパスの開始時にアタッチメントの内容をどのようにロードするかを指定する。
    // 選択肢としては、以前の内容を保持する vk::AttachmentLoadOp::eLoad、内容を破棄する vk::AttachmentLoadOp::eDiscard、内容を特定の値でクリアする vk::AttachmentLoadOp::eClear などが存在する。
    (*result)[0].loadOp = vk::AttachmentLoadOp::eClear;

    // storeOp: レンダーパスの終了時にアタッチメントの内容をどのように保存するかを指定する。選択肢としては、メモリに保存する 
    // vk::AttachmentStoreOp::eStore、内容を破棄する vk::AttachmentStoreOp::eDiscard などが存在する。
    (*result)[0].storeOp = vk::AttachmentStoreOp::eStore;

    // stencilLoadOp: ステンシルバッファに対するレンダーパス開始時のロード操作を指定する。loadOp と同様の選択肢が存在する。
    (*result)[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;

    // stencilStoreOp: ステンシルバッファに対するレンダーパス終了時のストア操作を指定する。storeOp と同様の選択肢が存在する。
    (*result)[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare; 

    // initialLayout: レンダーパス開始前の、アタッチメントの初期レイアウトを指定する。レイアウトは、GPU がどのようにメモリにアクセスするかを示すものであり、適切なレイアウトを設定することで効率的なレンダリングが可能となる。
    // 例えば、カラーアタッチメントであれば vk::ImageLayout::eUndefined や vk::ImageLayout::eColorAttachmentOptimal などが考えられる。
    (*result)[0].initialLayout = vk::ImageLayout::eUndefined;

    // イメージレイアウト
    // イメージのメモリ上における配置方法・取扱い方に関する指定
    // 以前は特に説明せずvk::ImageLayout::eGeneralを指定していた
    // 実に色々な設定があり、必要に応じて最適なものを指定しなければならない
    // 公式ドキュメントなどの資料を参考に正しいものを探すしかない
    // 間違ったものを指定するとエラーが出る場合がある
    // レンダーパスの設定によって、レンダリング処理が終わった後でどのようなイメージレイアウトにするかを決めることができる
    // 今回はレンダリングが終わった後で表示(プレゼン)しなければならず、その場合はvk::ImageLayout::ePresentSrcKHRでなければならないという決まりなのでこれを指定
    // ちなみにeGeneral形式のイメージは(最適ではないものの)一応どんな扱いを受けても基本的にはOKらしく、eGeneralのままでも動くには動く でもやめておいた方が無難
    // finalLayout: レンダーパス終了後の、アタッチメントの最終レイアウトを指定する。
    // レンダーパスの後にどのようにアタッチメントを使用するかによって適切なレイアウトを選択する必要がある。
    // 例えば、描画結果をスワップチェーンに表示する場合は vk::ImageLayout::ePresentSrcKHR、次のレンダーパスで入力として使用する場合は vk::ImageLayout::eShaderReadOnlyOptimal などが考えられる。
    (*result)[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    // レンダーパスは描画処理の大まかな流れを表すオブジェクト
    // 今までは画像一枚を出力するだけだったが、深度バッファが関わる場合は少し設定を変える必要がある
    // 具体的には、深度バッファもアタッチメントの一種という扱いなのでその設定をする
    (*result)[1].format = vk::Format::eD32Sfloat;
    (*result)[1].samples = vk::SampleCountFlagBits::e1;
    (*result)[1].loadOp = vk::AttachmentLoadOp::eClear;
    // storeOpはeDontCareにする
    // 深度バッファの最終的な値はどうでもいいため
    (*result)[1].storeOp = vk::AttachmentStoreOp::eDontCare;
    (*result)[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    (*result)[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    (*result)[1].initialLayout = vk::ImageLayout::eUndefined;
    // finalLayoutはeDepthStencilAttachmentOptimalを指定
    // 深度バッファとして使うイメージはこのレイアウトになっていると良いとされている
    (*result)[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    return result;
}

std::shared_ptr<vk::RenderPassCreateInfo> getRenderPassCreateInfo(std::vector<vk::AttachmentDescription> &attachmentDescriptions, std::vector<vk::SubpassDescription> &subpasses)
{
    std::shared_ptr<vk::RenderPassCreateInfo> result = std::make_shared<vk::RenderPassCreateInfo>();
    result->attachmentCount = attachmentDescriptions.size();
    result->pAttachments = attachmentDescriptions.data();
    result->subpassCount = subpasses.size();
    result->pSubpasses = subpasses.data();
    result->dependencyCount = 0;
    result->pDependencies = nullptr;
    return result;
}

std::shared_ptr<vk::UniqueRenderPass> getRenderPass(vk::UniqueDevice &device, vk::RenderPassCreateInfo &renderpassCreateInfo)
{
    // レンダーパスを作成

    // これでレンダーパスが作成できたが、
    // レンダーパスはあくまで「この処理はこのデータを相手にする、あの処理はあのデータを～」
    // という関係性を表す”枠組み”に過ぎず、それぞれの処理(＝サブパス)が具体的にどのような処理を行うかは関知しない
    // 実際にはいろいろなコマンドを任意の回数呼ぶことができる
    std::shared_ptr<vk::UniqueRenderPass> result = std::make_shared<vk::UniqueRenderPass>();
    *result = device.get().createRenderPassUnique(renderpassCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueRenderPass> getRenderPass(vk::UniqueDevice &device, vk::SurfaceFormatKHR &surfaceFormat, std::vector<vk::SubpassDescription> &subpasses)
{
    std::shared_ptr<std::vector<vk::AttachmentDescription>> attachmentDescriptions = getAttachmentDescriptions(surfaceFormat);
    std::shared_ptr<vk::RenderPassCreateInfo> renderPassCreateInfo = getRenderPassCreateInfo(*attachmentDescriptions, subpasses);
    return getRenderPass(device, *renderPassCreateInfo);
}