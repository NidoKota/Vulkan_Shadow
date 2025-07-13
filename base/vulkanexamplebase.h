/*
* Vulkan �T���v�� ���N���X
*
* Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
*
* ���̃R�[�h��MIT���C�Z���X�iMIT�j(http://opensource.org/licenses/MIT)�̉��Ń��C�Z���X����Ă��܂��B
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
	// glsl�܂���hlsl�V�F�[�_�[�f�B���N�g���̃��[�g�ւ̃p�X��Ԃ��܂��B
	std::string getShadersPath() const;

	// fps��\�����邽�߂̃t���[���J�E���^�[
	uint32_t frameCounter = 0;
	uint32_t lastFPS = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp, tPrevEnd;
	// Vulkan�C���X�^���X�A���ׂẴA�v���P�[�V�������Ƃ̏�Ԃ��i�[���܂��B
	VkInstance instance{ VK_NULL_HANDLE };
	std::vector<std::string> supportedInstanceExtensions;
	// Vulkan���g�p���镨���f�o�C�X�iGPU�j
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
	// �����f�o�C�X�̃v���p�e�B���i�[���܂��i��F�f�o�C�X�̐������`�F�b�N���邽�߁j�B
	VkPhysicalDeviceProperties deviceProperties{};
	// �I�����ꂽ�����f�o�C�X�ŗ��p�\�ȋ@�\���i�[���܂��i��F�@�\�����p�\���`�F�b�N���邽�߁j�B
	VkPhysicalDeviceFeatures deviceFeatures{};
	// �����f�o�C�X�ŗ��p�\�Ȃ��ׂẴ������i�^�C�v�j�v���p�e�B���i�[���܂��B
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
	/** @brief ���̃T���v���ŗL���ɂ��镨���f�o�C�X�̋@�\�Z�b�g�i�h���N���X�̃R���X�g���N�^�Őݒ肷��K�v������܂��j*/
	VkPhysicalDeviceFeatures enabledFeatures{};
	/** @brief ���̃T���v���ŗL���ɂ���f�o�C�X�g���@�\�̃Z�b�g�i�h���N���X�̃R���X�g���N�^�Őݒ肷��K�v������܂��j*/
	std::vector<const char*> enabledDeviceExtensions;
	std::vector<const char*> enabledInstanceExtensions;
	/** @brief �f�o�C�X�쐬���Ɋg���@�\�̍\���̂�n�����߂̃I�v�V������pNext�\���� */
	void* deviceCreatepNextChain = nullptr;
	/** @brief �_���f�o�C�X�A�A�v���P�[�V�������猩�������f�o�C�X�iGPU�j*/
	VkDevice device{ VK_NULL_HANDLE };
	// �R�}���h�o�b�t�@���T�u�~�b�g�����f�o�C�X�̃O���t�B�b�N�X�L���[�ւ̃n���h��
	VkQueue queue{ VK_NULL_HANDLE };
	// �[�x�o�b�t�@�̃t�H�[�}�b�g�iVulkan�̏��������ɑI������܂��j
	VkFormat depthFormat;
	// �R�}���h�o�b�t�@�v�[��
	VkCommandPool cmdPool{ VK_NULL_HANDLE };
	/** @brief �O���t�B�b�N�X�L���[�ւ̃T�u�~�b�V������ҋ@���邽�߂Ɏg�p�����p�C�v���C���X�e�[�W */
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// �L���[�ɒ񎦂����R�}���h�o�b�t�@�ƃZ�}�t�H���܂݂܂��B
	VkSubmitInfo submitInfo;
	// �����_�����O�Ɏg�p�����R�}���h�o�b�t�@
	std::vector<VkCommandBuffer> drawCmdBuffers;
	// �t���[���o�b�t�@�ւ̏������݂̂��߂̃O���[�o�������_�[�p�X
	VkRenderPass renderPass{ VK_NULL_HANDLE };
	// ���p�\�ȃt���[���o�b�t�@�̃��X�g�i�X���b�v�`�F�[���̃C���[�W���Ɠ����j
	std::vector<VkFramebuffer>frameBuffers;
	// �A�N�e�B�u�ȃt���[���o�b�t�@�̃C���f�b�N�X
	uint32_t currentBuffer = 0;
	// �f�B�X�N���v�^�Z�b�g�v�[��
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
	// �쐬���ꂽ�V�F�[�_�[���W���[���̃��X�g�i�N���[���A�b�v�̂��߂ɕۑ�����܂��j
	std::vector<VkShaderModule> shaderModules;
	// �p�C�v���C���L���b�V���I�u�W�F�N�g
	VkPipelineCache pipelineCache{ VK_NULL_HANDLE };
	// �C���[�W�i�t���[���o�b�t�@�j���E�B���h�E�V�X�e���ɒ񎦂��邽�߂̃X���b�v�`�F�[�������b�v���܂��B
	VulkanSwapChain swapChain;
	// �����Z�}�t�H
	struct {
		// �X���b�v�`�F�[���C���[�W�̒�
		VkSemaphore presentComplete;
		// �R�}���h�o�b�t�@�̃T�u�~�b�g�Ǝ��s
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

	/** @brief �����\�^�C�}�[�i���p�\�ȏꍇ�j���g�p���đ��肳�ꂽ�Ō�̃t���[������ */
	float frameTimer = 1.0f;

	vks::Benchmark benchmark;

	/** @brief �J�v�Z�������ꂽ��������ј_��Vulkan�f�o�C�X */
	vks::VulkanDevice* vulkanDevice;

	/** @brief �R�}���h���C�������ȂǂŕύX�\�ȃT���v���̐ݒ� */
	struct Settings {
		/** @brief true�ɐݒ肷��ƁA���؃��C���[�i����у��b�Z�[�W�o�́j��L���ɂ��܂� */
		bool validation = false;
		/** @brief �R�}���h���C���Ńt���X�N���[�����[�h���v�����ꂽ�ꍇ��true�ɐݒ肵�܂� */
		bool fullscreen = false;
		/** @brief �X���b�v�`�F�[����v-sync�����������ꍇ��true�ɐݒ肵�܂� */
		bool vsync = false;
		/** @brief UI�I�[�o�[���C��L���ɂ��܂� */
		bool overlay = true;
	} settings;

	/** @brief �Q�[���p�b�h���͂̏�ԁiAndroid�ł̂ݎg�p�j*/
	struct {
		glm::vec2 axisLeft = glm::vec2(0.0f);
		glm::vec2 axisRight = glm::vec2(0.0f);
	} gamePadState;

	/** @brief �}�E�X/�^�b�`���͂̏�� */
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

	// -1.0����1.0�ɃN�����v���ꂽ�A�t���[�����[�g�Ɉˑ����Ȃ��^�C�}�[�l���`���܂��B
	// �A�j���[�V�������]�ȂǂŎg�p���܂��B
	float timer = 0.0f;
	// �O���[�o���^�C�}�[�𑬂��i�܂��͒x���j���邽�߂̏搔
	float timerSpeed = 0.25f;
	bool paused = false;

	Camera camera;

	std::string title = "Vulkan Example";
	std::string name = "vulkanExample";
	uint32_t apiVersion = VK_API_VERSION_1_0;

	/** @brief �f�t�H���g�̃����_�[�p�X�Ŏg�p�����A�f�t�H���g�̐[�x/�X�e���V���A�^�b�`�����g */
	struct {
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
	} depthStencil{};

	// OS�ŗL
#if defined(_WIN32)
	HWND window;
	HINSTANCE windowInstance;
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	// �A�v���P�[�V�������t�H�[�J�X����Ă���ꍇ��true�A�o�b�N�O���E���h�Ɉړ������ꍇ��false
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

	/** @brief �f�t�H���g�̊��N���X�̃R���X�g���N�^ */
	VulkanExampleBase();
	virtual ~VulkanExampleBase();
	/** @brief Vulkan�C���X�^���X���Z�b�g�A�b�v���A�K�v�Ȋg���@�\��L���ɂ��A�����f�o�C�X�iGPU�j�ɐڑ����܂� */
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
	/** @brief (���z) �A�v���P�[�V�����S�̂�Vulkan�C���X�^���X���쐬���܂� */
	virtual VkResult createInstance();
	/** @brief (�������z) �T���v���A�v���P�[�V�����Ŏ�������郌���_�[�֐� */
	virtual void render() = 0;
	/** @brief (���z) �L�[�������ꂽ��ɌĂяo����A�J�X�^���L�[�n���h�����O�Ɏg�p�ł��܂� */
	virtual void keyPressed(uint32_t);
	/** @brief (���z) �}�E�X�J�[�\�����ړ�������A�����C�x���g�i�J�����̉�]�Ȃǁj�����������O�ɌĂяo����܂� */
	virtual void mouseMoved(double x, double y, bool& handled);
	/** @brief (���z) �E�B���h�E�����T�C�Y���ꂽ�Ƃ��ɌĂяo����A�T���v���A�v���P�[�V�����Ń��\�[�X���č쐬���邽�߂Ɏg�p�ł��܂� */
	virtual void windowResized();
	/** @brief (���z) �R�}���h�o�b�t�@�̍č\�z���K�v�ȃ��\�[�X�i��F�t���[���o�b�t�@�j���č쐬���ꂽ�Ƃ��ɌĂяo����܂��B�T���v���A�v���P�[�V�����Ŏ�������܂� */
	virtual void buildCommandBuffers();
	/** @brief (���z) �f�t�H���g�̐[�x�r���[�ƃX�e���V���r���[���Z�b�g�A�b�v���܂� */
	virtual void setupDepthStencil();
	/** @brief (���z) �v�����ꂽ���ׂẴX���b�v�`�F�[���C���[�W�ɑ΂��ăf�t�H���g�̃t���[���o�b�t�@���Z�b�g�A�b�v���܂� */
	virtual void setupFrameBuffer();
	/** @brief (���z) �f�t�H���g�̃����_�[�p�X���Z�b�g�A�b�v���܂� */
	virtual void setupRenderPass();
	/** @brief (���z) �����f�o�C�X�̋@�\���ǂݎ��ꂽ��ɌĂяo����A�f�o�C�X�ŗL���ɂ���@�\�̐ݒ�Ɏg�p�ł��܂� */
	virtual void getEnabledFeatures();
	/** @brief (���z) �����f�o�C�X�̊g���@�\���ǂݎ��ꂽ��ɌĂяo����A�T�|�[�g����Ă���g���@�\���X�g�Ɋ�Â��Ċg���@�\��L���ɂ��邽�߂Ɏg�p�ł��܂� */
	virtual void getEnabledExtensions();

	/** @brief �T���v�������s���邽�߂ɕK�v�Ȃ��ׂĂ�Vulkan���\�[�X�Ɗ֐����������܂� */
	virtual void prepare();

	/** @brief �w�肳�ꂽ�V�F�[�_�[�X�e�[�W��SPIR-V�V�F�[�_�[�t�@�C�������[�h���܂� */
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

	void windowResize();

	/** @brief ���C�������_�[���[�v�̃G���g���[�|�C���g */
	void renderLoop();

	/** @brief ImGui�I�[�o�[���C�̕`��R�}���h���A�w�肳�ꂽ�R�}���h�o�b�t�@�ɒǉ����܂� */
	void drawUI(const VkCommandBuffer commandBuffer);

	/** ���̃X���b�v�`�F�[���C���[�W���擾���āA���̃t���[���̃��[�N���[�h�T�u�~�b�V�������������܂� */
	void prepareFrame();
	/** @brief ���݂̃C���[�W���X���b�v�`�F�[���ɒ񎦂��܂� */
	void submitFrame();
	/** @brief (���z) �f�t�H���g�̃C���[�W�擾+�T�u�~�b�V��������уR�}���h�o�b�t�@�T�u�~�b�V�����֐� */
	virtual void renderFrame();

	/** @brief (���z) UI�I�[�o�[���C���X�V�����Ƃ��ɌĂяo����A�I�[�o�[���C�ɃJ�X�^���v�f��ǉ����邽�߂Ɏg�p�ł��܂� */
	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);

#if defined(_WIN32)
	virtual void OnHandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
};

#include "Entrypoints.h"