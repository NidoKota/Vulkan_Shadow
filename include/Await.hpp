#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

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