/*
* Vulkan サンプル 基底クラス
*
* Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
*
* このコードはMITライセンス（MIT）(http://opensource.org/licenses/MIT)の下でライセンスされています。
*/

#pragma once

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <ShellScalingAPI.h>
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>
#include "VulkanAndroid.h"
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
#include <directfb.h>
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#elif defined(_DIRECT2DISPLAY)
//
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <numeric>
#include <ctime>
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include <sys/stat.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <numeric>
#include <array>

#include "vulkan/vulkan.h"

#include "CommandLineParser.hpp"
#include "keycodes.hpp"
#include "VulkanTools.h"
#include "VulkanDebug.h"
#include "VulkanUIOverlay.h"
#include "VulkanSwapChain.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"

#include "VulkanInitializers.hpp"
#include "camera.hpp"
#include "benchmark.hpp"

class VulkanExampleBase
{
private:
	std::string getWindowTitle();
	uint32_t destWidth;
	uint32_t destHeight;
	bool resizing = false;
	void handleMouseMove(int32_t x, int32_t y);
	void nextFrame();
	void updateOverlay();
	void createPipelineCache();
	void createCommandPool();
	void createSynchronizationPrimitives();
	void initSwapchain();
	void setupSwapChain();
	void createCommandBuffers();
	void destroyCommandBuffers();
	std::string shaderDir = "glsl";
protected:
	// glslまたはhlslシェーダーディレクトリのルートへのパスを返します。
	std::string getShadersPath() const;

	// fpsを表示するためのフレームカウンター
	uint32_t frameCounter = 0;
	uint32_t lastFPS = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp, tPrevEnd;
	// Vulkanインスタンス、すべてのアプリケーションごとの状態を格納します。
	VkInstance instance{ VK_NULL_HANDLE };
	std::vector<std::string> supportedInstanceExtensions;
	// Vulkanが使用する物理デバイス（GPU）
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
	// 物理デバイスのプロパティを格納します（例：デバイスの制限をチェックするため）。
	VkPhysicalDeviceProperties deviceProperties{};
	// 選択された物理デバイスで利用可能な機能を格納します（例：機能が利用可能かチェックするため）。
	VkPhysicalDeviceFeatures deviceFeatures{};
	// 物理デバイスで利用可能なすべてのメモリ（タイプ）プロパティを格納します。
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
	/** @brief このサンプルで有効にする物理デバイスの機能セット（派生クラスのコンストラクタで設定する必要があります）*/
	VkPhysicalDeviceFeatures enabledFeatures{};
	/** @brief このサンプルで有効にするデバイス拡張機能のセット（派生クラスのコンストラクタで設定する必要があります）*/
	std::vector<const char*> enabledDeviceExtensions;
	std::vector<const char*> enabledInstanceExtensions;
	/** @brief デバイス作成時に拡張機能の構造体を渡すためのオプションのpNext構造体 */
	void* deviceCreatepNextChain = nullptr;
	/** @brief 論理デバイス、アプリケーションから見た物理デバイス（GPU）*/
	VkDevice device{ VK_NULL_HANDLE };
	// コマンドバッファがサブミットされるデバイスのグラフィックスキューへのハンドル
	VkQueue queue{ VK_NULL_HANDLE };
	// 深度バッファのフォーマット（Vulkanの初期化中に選択されます）
	VkFormat depthFormat;
	// コマンドバッファプール
	VkCommandPool cmdPool{ VK_NULL_HANDLE };
	/** @brief グラフィックスキューへのサブミッションを待機するために使用されるパイプラインステージ */
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// キューに提示されるコマンドバッファとセマフォを含みます。
	VkSubmitInfo submitInfo;
	// レンダリングに使用されるコマンドバッファ
	std::vector<VkCommandBuffer> drawCmdBuffers;
	// フレームバッファへの書き込みのためのグローバルレンダーパス
	VkRenderPass renderPass{ VK_NULL_HANDLE };
	// 利用可能なフレームバッファのリスト（スワップチェーンのイメージ数と同じ）
	std::vector<VkFramebuffer>frameBuffers;
	// アクティブなフレームバッファのインデックス
	uint32_t currentBuffer = 0;
	// ディスクリプタセットプール
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
	// 作成されたシェーダーモジュールのリスト（クリーンアップのために保存されます）
	std::vector<VkShaderModule> shaderModules;
	// パイプラインキャッシュオブジェクト
	VkPipelineCache pipelineCache{ VK_NULL_HANDLE };
	// イメージ（フレームバッファ）をウィンドウシステムに提示するためのスワップチェーンをラップします。
	VulkanSwapChain swapChain;
	// 同期セマフォ
	struct {
		// スワップチェーンイメージの提示
		VkSemaphore presentComplete;
		// コマンドバッファのサブミットと実行
		VkSemaphore renderComplete;
	} semaphores;
	std::vector<VkFence> waitFences;
	bool requiresStencil{ false };
public:
	bool prepared = false;
	bool resized = false;
	bool viewUpdated = false;
	uint32_t width = 1280;
	uint32_t height = 720;

	vks::UIOverlay ui;
	CommandLineParser commandLineParser;

	/** @brief 高性能タイマー（利用可能な場合）を使用して測定された最後のフレーム時間 */
	float frameTimer = 1.0f;

	vks::Benchmark benchmark;

	/** @brief カプセル化された物理および論理Vulkanデバイス */
	vks::VulkanDevice* vulkanDevice;

	/** @brief コマンドライン引数などで変更可能なサンプルの設定 */
	struct Settings {
		/** @brief trueに設定すると、検証レイヤー（およびメッセージ出力）を有効にします */
		bool validation = false;
		/** @brief コマンドラインでフルスクリーンモードが要求された場合にtrueに設定します */
		bool fullscreen = false;
		/** @brief スワップチェーンでv-syncが強制される場合にtrueに設定します */
		bool vsync = false;
		/** @brief UIオーバーレイを有効にします */
		bool overlay = true;
	} settings;

	/** @brief ゲームパッド入力の状態（Androidでのみ使用）*/
	struct {
		glm::vec2 axisLeft = glm::vec2(0.0f);
		glm::vec2 axisRight = glm::vec2(0.0f);
	} gamePadState;

	/** @brief マウス/タッチ入力の状態 */
	struct {
		struct {
			bool left = false;
			bool right = false;
			bool middle = false;
		} buttons;
		glm::vec2 position;
	} mouseState;

	VkClearColorValue defaultClearColor = { { 0.025f, 0.025f, 0.025f, 1.0f } };

	static std::vector<const char*> args;

	// -1.0から1.0にクランプされた、フレームレートに依存しないタイマー値を定義します。
	// アニメーションや回転などで使用します。
	float timer = 0.0f;
	// グローバルタイマーを速く（または遅く）するための乗数
	float timerSpeed = 0.25f;
	bool paused = false;

	Camera camera;

	std::string title = "Vulkan Example";
	std::string name = "vulkanExample";
	uint32_t apiVersion = VK_API_VERSION_1_0;

	/** @brief デフォルトのレンダーパスで使用される、デフォルトの深度/ステンシルアタッチメント */
	struct {
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
	} depthStencil{};

	// OS固有
#if defined(_WIN32)
	HWND window;
	HINSTANCE windowInstance;
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	// アプリケーションがフォーカスされている場合はtrue、バックグラウンドに移動した場合はfalse
	bool focused = false;
	struct TouchPos {
		int32_t x;
		int32_t y;
	} touchPos;
	bool touchDown = false;
	double touchTimer = 0.0;
	int64_t lastTapTime = 0;
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
	void* view;
#if defined(VK_USE_PLATFORM_METAL_EXT)
	CAMetalLayer* metalLayer;
#endif
#if defined(VK_EXAMPLE_XCODE_GENERATED)
	bool quit = false;
#endif
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
	bool quit = false;
	IDirectFB* dfb = nullptr;
	IDirectFBDisplayLayer* layer = nullptr;
	IDirectFBWindow* window = nullptr;
	IDirectFBSurface* surface = nullptr;
	IDirectFBEventBuffer* event_buffer = nullptr;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	wl_display* display = nullptr;
	wl_registry* registry = nullptr;
	wl_compositor* compositor = nullptr;
	struct xdg_wm_base* shell = nullptr;
	wl_seat* seat = nullptr;
	wl_pointer* pointer = nullptr;
	wl_keyboard* keyboard = nullptr;
	wl_surface* surface = nullptr;
	struct xdg_surface* xdg_surface;
	struct xdg_toplevel* xdg_toplevel;
	bool quit = false;
	bool configured = false;

#elif defined(_DIRECT2DISPLAY)
	bool quit = false;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	bool quit = false;
	xcb_connection_t* connection;
	xcb_screen_t* screen;
	xcb_window_t window;
	xcb_intern_atom_reply_t* atom_wm_delete_window;
#elif defined(VK_USE_PLATFORM_HEADLESS_EXT)
	bool quit = false;
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
	screen_context_t screen_context = nullptr;
	screen_window_t screen_window = nullptr;
	screen_event_t screen_event = nullptr;
	bool quit = false;
#endif

	/** @brief デフォルトの基底クラスのコンストラクタ */
	VulkanExampleBase();
	virtual ~VulkanExampleBase();
	/** @brief Vulkanインスタンスをセットアップし、必要な拡張機能を有効にし、物理デバイス（GPU）に接続します */
	bool initVulkan();

#if defined(_WIN32)
	void setupConsole(std::string title);
	void setupDPIAwareness();
	HWND setupWindow(HINSTANCE hinstance, WNDPROC wndproc);
	void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	static int32_t handleAppInput(struct android_app* app, AInputEvent* event);
	static void handleAppCommand(android_app* app, int32_t cmd);
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
	void* setupWindow(void* view);
	void displayLinkOutputCb();
	void mouseDragged(float x, float y);
	void windowWillResize(float x, float y);
	void windowDidResize();
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
	IDirectFBSurface* setupWindow();
	void handleEvent(const DFBWindowEvent* event);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	struct xdg_surface* setupWindow();
	void initWaylandConnection();
	void setSize(int width, int height);
	static void registryGlobalCb(void* data, struct wl_registry* registry,
		uint32_t name, const char* interface, uint32_t version);
	void registryGlobal(struct wl_registry* registry, uint32_t name,
		const char* interface, uint32_t version);
	static void registryGlobalRemoveCb(void* data, struct wl_registry* registry,
		uint32_t name);
	static void seatCapabilitiesCb(void* data, wl_seat* seat, uint32_t caps);
	void seatCapabilities(wl_seat* seat, uint32_t caps);
	static void pointerEnterCb(void* data, struct wl_pointer* pointer,
		uint32_t serial, struct wl_surface* surface, wl_fixed_t sx,
		wl_fixed_t sy);
	static void pointerLeaveCb(void* data, struct wl_pointer* pointer,
		uint32_t serial, struct wl_surface* surface);
	static void pointerMotionCb(void* data, struct wl_pointer* pointer,
		uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
	void pointerMotion(struct wl_pointer* pointer,
		uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
	static void pointerButtonCb(void* data, struct wl_pointer* wl_pointer,
		uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
	void pointerButton(struct wl_pointer* wl_pointer,
		uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
	static void pointerAxisCb(void* data, struct wl_pointer* wl_pointer,
		uint32_t time, uint32_t axis, wl_fixed_t value);
	void pointerAxis(struct wl_pointer* wl_pointer,
		uint32_t time, uint32_t axis, wl_fixed_t value);
	static void keyboardKeymapCb(void* data, struct wl_keyboard* keyboard,
		uint32_t format, int fd, uint32_t size);
	static void keyboardEnterCb(void* data, struct wl_keyboard* keyboard,
		uint32_t serial, struct wl_surface* surface, struct wl_array* keys);
	static void keyboardLeaveCb(void* data, struct wl_keyboard* keyboard,
		uint32_t serial, struct wl_surface* surface);
	static void keyboardKeyCb(void* data, struct wl_keyboard* keyboard,
		uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
	void keyboardKey(struct wl_keyboard* keyboard,
		uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
	static void keyboardModifiersCb(void* data, struct wl_keyboard* keyboard,
		uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
		uint32_t mods_locked, uint32_t group);

#elif defined(_DIRECT2DISPLAY)
	//
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	xcb_window_t setupWindow();
	void initxcbConnection();
	void handleEvent(const xcb_generic_event_t* event);
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
	void setupWindow();
	void handleEvent();
#else
	void setupWindow();
#endif
	/** @brief (仮想) アプリケーション全体のVulkanインスタンスを作成します */
	virtual VkResult createInstance();
	/** @brief (純粋仮想) サンプルアプリケーションで実装されるレンダー関数 */
	virtual void render() = 0;
	/** @brief (仮想) キーが押された後に呼び出され、カスタムキーハンドリングに使用できます */
	virtual void keyPressed(uint32_t);
	/** @brief (仮想) マウスカーソルが移動した後、内部イベント（カメラの回転など）が処理される前に呼び出されます */
	virtual void mouseMoved(double x, double y, bool& handled);
	/** @brief (仮想) ウィンドウがリサイズされたときに呼び出され、サンプルアプリケーションでリソースを再作成するために使用できます */
	virtual void windowResized();
	/** @brief (仮想) コマンドバッファの再構築が必要なリソース（例：フレームバッファ）が再作成されたときに呼び出されます。サンプルアプリケーションで実装されます */
	virtual void buildCommandBuffers();
	/** @brief (仮想) デフォルトの深度ビューとステンシルビューをセットアップします */
	virtual void setupDepthStencil();
	/** @brief (仮想) 要求されたすべてのスワップチェーンイメージに対してデフォルトのフレームバッファをセットアップします */
	virtual void setupFrameBuffer();
	/** @brief (仮想) デフォルトのレンダーパスをセットアップします */
	virtual void setupRenderPass();
	/** @brief (仮想) 物理デバイスの機能が読み取られた後に呼び出され、デバイスで有効にする機能の設定に使用できます */
	virtual void getEnabledFeatures();
	/** @brief (仮想) 物理デバイスの拡張機能が読み取られた後に呼び出され、サポートされている拡張機能リストに基づいて拡張機能を有効にするために使用できます */
	virtual void getEnabledExtensions();

	/** @brief サンプルを実行するために必要なすべてのVulkanリソースと関数を準備します */
	virtual void prepare();

	/** @brief 指定されたシェーダーステージのSPIR-Vシェーダーファイルをロードします */
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

	void windowResize();

	/** @brief メインレンダーループのエントリーポイント */
	void renderLoop();

	/** @brief ImGuiオーバーレイの描画コマンドを、指定されたコマンドバッファに追加します */
	void drawUI(const VkCommandBuffer commandBuffer);

	/** 次のスワップチェーンイメージを取得して、次のフレームのワークロードサブミッションを準備します */
	void prepareFrame();
	/** @brief 現在のイメージをスワップチェーンに提示します */
	void submitFrame();
	/** @brief (仮想) デフォルトのイメージ取得+サブミッションおよびコマンドバッファサブミッション関数 */
	virtual void renderFrame();

	/** @brief (仮想) UIオーバーレイが更新されるときに呼び出され、オーバーレイにカスタム要素を追加するために使用できます */
	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);

#if defined(_WIN32)
	virtual void OnHandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
};

#include "Entrypoints.h"