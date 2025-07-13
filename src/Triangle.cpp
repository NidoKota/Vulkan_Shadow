/*
* Vulkan�T���v�� - ��{�I�ȃC���f�b�N�X�t���O�p�`�̃����_�����O
*
* ���ӁF
*	�����Vulkan�𗧂��グ�ĉ�����\��������@���������߂́A����΁u�x�^���݁v�̃T���v���ł��B
*	���̃T���v���Ƃ͑ΏƓI�ɁA���̃T���v���ł́i�X���b�v�`�F�[���̃Z�b�g�A�b�v�Ȃǂ������j�w���p�[�֐��⏉�����q�͎g�p���܂���B
*
* Copyright (C) 2016-2023 by Sascha Willems - www.saschawillems.de
*
* ���̃R�[�h��MIT���C�Z���X�iMIT�j(http://opensource.org/licenses/MIT)�̉��Ń��C�Z���X����Ă��܂��B
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <exception>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "../base/vulkanexamplebase.h"

// GPU��CPU����Ƀr�W�[��Ԃɕۂ������B���̂��߂ɁA�O�̃R�}���h�o�b�t�@���܂����s���ł����Ă��A�V�����R�}���h�o�b�t�@�̍\�z���J�n���邱�Ƃ�����܂��B
// ���̐��́A�����ɏ����ł���t���[���̍ő吔���`���܂��B
// ���̐��𑝂₷�ƃp�t�H�[�}���X�����シ��\��������܂����A�ǉ��̒x�����������܂��B
#define MAX_CONCURRENT_FRAMES 2

class VulkanExample : public VulkanExampleBase
{
public:
	// ���̃T���v���Ŏg�p����钸�_���C�A�E�g
	struct Vertex {
		float position[3];
		float color[3];
	};

	// ���_�o�b�t�@�Ƒ���
	struct {
		VkDeviceMemory memory{ VK_NULL_HANDLE }; // ���̃o�b�t�@�̂��߂̃f�o�C�X�������ւ̃n���h��
		VkBuffer buffer;						 // ���������o�C���h����Ă���Vulkan�o�b�t�@�I�u�W�F�N�g�ւ̃n���h��
	} vertices;

	// �C���f�b�N�X�o�b�t�@
	struct {
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer buffer;
		uint32_t count{ 0 };
	} indices;

	// ���j�t�H�[���o�b�t�@�u���b�N�I�u�W�F�N�g
	struct UniformBuffer {
		VkDeviceMemory memory;
		VkBuffer buffer;
		// �f�B�X�N���v�^�Z�b�g�́A�V�F�[�_�[�̃o�C���f�B���O�|�C���g�Ƀo�C���h���ꂽ���\�[�X���i�[���܂��B
		// ����́A�قȂ�V�F�[�_�[�̃o�C���f�B���O�|�C���g���A�����̃o�C���f�B���O�Ɏg�p�����o�b�t�@��C���[�W�ɐڑ����܂��B
		VkDescriptorSet descriptorSet;
		// �}�b�v���ꂽ�o�b�t�@�ւ̃|�C���^��ێ����Ă������ƂŁAmemcpy����Ă��̓��e���ȒP�ɍX�V�ł��܂��B
		uint8_t* mapped{ nullptr };
	};
	// �t���[�����Ƃ�1��UBO���g�p���邱�ƂŁA�t���[���̃I�[�o�[���b�v���\�ɂ��A���j�t�H�[�����܂��g�p���ɍX�V����Ȃ��悤�ɂ��܂��B
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffers;

	// �ȒP�ɂ��邽�߂ɁA�V�F�[�_�[�Ɠ������j�t�H�[���u���b�N���C�A�E�g���g�p���܂��F
	//
	//	layout(set = 0, binding = 0) uniform UBO
	//	{
	//		mat4 projectionMatrix;
	//		mat4 modelMatrix;
	//		mat4 viewMatrix;
	//	} ubo;
	//
	// ���̂悤�ɂ��邱�ƂŁAubo�f�[�^�����̂܂�ubo��memcpy�ł��܂��B
	// ���ӁF�蓮�ł̃p�f�B���O������邽�߂ɁAGPU�ƃA���C�����g�������f�[�^�^�ivec4, mat4�j���g�p����K�v������܂��B
	struct ShaderData {
		glm::mat4 projectionMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
	};

	// �p�C�v���C�����C�A�E�g�́A�p�C�v���C�����f�B�X�N���v�^�Z�b�g�ɃA�N�Z�X���邽�߂Ɏg�p����܂��B
	// ����́A�V�F�[�_�[�X�e�[�W�ƃV�F�[�_�[���\�[�X�Ԃ̃C���^�[�t�F�[�X���i���ۂ̃f�[�^���o�C���h�����Ɂj��`���܂��B
	// �p�C�v���C�����C�A�E�g�́A�C���^�[�t�F�[�X����v�������A�����̃p�C�v���C���Ԃŋ��L�ł��܂��B
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

	// �p�C�v���C���i���΂��΁u�p�C�v���C���X�e�[�g�I�u�W�F�N�g�v�ƌĂ΂��j�́A�p�C�v���C���ɉe����^���邷�ׂẴX�e�[�g���u�Ă��t����v���߂Ɏg�p����܂��B
	// OpenGL�ł͂��ׂẴX�e�[�g���i�قځj���ł��ύX�ł��܂������AVulkan�ł̓O���t�B�b�N�X�i����уR���s���[�g�j�p�C�v���C���̃X�e�[�g�����O�Ƀ��C�A�E�g����K�v������܂��B
	// ���̂��߁A���I�łȂ��p�C�v���C���X�e�[�g�̑g�ݍ��킹���ƂɁA�V�����p�C�v���C�����K�v�ɂȂ�܂��i�����ł͐������Ȃ��������̗�O������܂��j�B
	// ����͎��O�̌v��Ƃ����V���Ȏ�����ǉ����܂����A�h���C�o�[�ɂ��p�t�H�[�}���X�œK���̐�D�̋@��ł�����܂��B
	VkPipeline pipeline{ VK_NULL_HANDLE };

	// �f�B�X�N���v�^�Z�b�g���C�A�E�g�́A�V�F�[�_�[�̃o�C���f�B���O���C�A�E�g���i���ۂɃf�B�X�N���v�^���Q�Ƃ����Ɂj�L�q���܂��B
	// �p�C�v���C�����C�A�E�g�Ɠ��l�ɁA����͂قڐ݌v�}�̂悤�Ȃ��̂ł���A���C�A�E�g����v�������A�قȂ�f�B�X�N���v�^�Z�b�g�Ŏg�p�ł��܂��B
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };

	// �����v���~�e�B�u
	// ������Vulkan�̏d�v�ȊT�O�ł���AOpenGL�ł͂قƂ�ǉB����Ă��܂����B����𐳂����s�����Ƃ�Vulkan���g�p�����Ŕ��ɏd�v�ł��B

	// �Z�}�t�H�́A�O���t�B�b�N�X�L���[���̑���𒲐����A�������R�}���h������ۏ؂��邽�߂Ɏg�p����܂��B
	std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> presentCompleteSemaphores{};
	std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> renderCompleteSemaphores{};

	VkCommandPool commandPool{ VK_NULL_HANDLE };
	std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES> commandBuffers{};
	std::array<VkFence, MAX_CONCURRENT_FRAMES> waitFences{};

	// �����������I�u�W�F�N�g��I�����邽�߂ɁA���݂̃t���[����ǐՂ���K�v������܂��B
	uint32_t currentFrame{ 0 };

	VulkanExample() : VulkanExampleBase()
	{
		title = "Vulkan Example - Basic indexed triangle";
		// �ȒP�ɂ��邽�߂ɁA�t���[�����[�N��UI�I�[�o�[���C�͎g�p���܂���B
		settings.overlay = false;
		// �f�t�H���g��look-at�J�������Z�b�g�A�b�v���܂��B
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));
		camera.setRotation(glm::vec3(0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 1.0f, 256.0f);
		// �����Őݒ肳��Ă��Ȃ��l�́A���N���X�̃R���X�g���N�^�ŏ���������܂��B
	}

	~VulkanExample()
	{
		// �g�p����Vulkan���\�[�X���N���[���A�b�v���܂��B
		// ���ӁF�p�����ꂽ�f�X�g���N�^�����N���X�Ɋi�[����Ă��郊�\�[�X���N���[���A�b�v���܂��B
		vkDestroyPipeline(device, pipeline, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, vertices.buffer, nullptr);
		vkFreeMemory(device, vertices.memory, nullptr);

		vkDestroyBuffer(device, indices.buffer, nullptr);
		vkFreeMemory(device, indices.memory, nullptr);

		vkDestroyCommandPool(device, commandPool, nullptr);

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			vkDestroyFence(device, waitFences[i], nullptr);
			vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
			vkDestroyBuffer(device, uniformBuffers[i].buffer, nullptr);
			vkFreeMemory(device, uniformBuffers[i].memory, nullptr);
		}
	}

	// ���̊֐��́A�v�����邷�ׂẴv���p�e�B�t���O�i��F�f�o�C�X���[�J���A�z�X�g���j���T�|�[�g����f�o�C�X�������^�C�v��v�����邽�߂Ɏg�p����܂��B
	// ��������ƁA�v�����ꂽ�������v���p�e�B�ɓK�����郁�����^�C�v�̃C���f�b�N�X��Ԃ��܂��B
	// ����́A�������قȂ郁�����v���p�e�B�����C�ӂ̐��̃������^�C�v��񋟂���\�������邽�߁A�K�v�ł��B
	// ���܂��܂ȃ������\���̏ڍׂɂ��ẮAhttps://vulkan.gpuinfo.org/ ���m�F���Ă��������B
	uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
	{
		// ���̃T���v���Ŏg�p����Ă���f�o�C�X�ŗ��p�\�Ȃ��ׂẴ������^�C�v���C�e���[�g���܂��B
		for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}
			typeBits >>= 1;
		}

		throw "Could not find a suitable memory type!";
	}

	// ���̃T���v���Ŏg�p�����t���[�����Ƃ́iin flight�jVulkan�����v���~�e�B�u���쐬���܂��B
	void createSynchronizationPrimitives()
	{
		// �Z�}�t�H�́A�L���[���ł̐������R�}���h�����̂��߂Ɏg�p����܂��B
		VkSemaphoreCreateInfo semaphoreCI{};
		semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// �t�F���X�́A�z�X�g���ŕ`��R�}���h�o�b�t�@�̊������m�F���邽�߂Ɏg�p����܂��B
		VkFenceCreateInfo fenceCI{};
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// �t�F���X���V�O�i����Ԃō쐬���܂��i����ɂ��A�e�R�}���h�o�b�t�@�̍ŏ��̃����_�����O�őҋ@���Ȃ��Ȃ�܂��j�B
		fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			// �ēx�T�u�~�b�g���J�n����O�ɁA�C���[�W�̒񎦂��������Ă��邱�Ƃ�ۏ؂��邽�߂Ɏg�p�����Z�}�t�H�B
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &presentCompleteSemaphores[i]));
			// �C���[�W���L���[�ɃT�u�~�b�g����O�ɁA�T�u�~�b�g���ꂽ���ׂẴR�}���h���I���������Ƃ�ۏ؂��邽�߂Ɏg�p�����Z�}�t�H�B
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &renderCompleteSemaphores[i]));

			// �R�}���h�o�b�t�@���ēx�g�p����O�ɁA���̎��s�������������Ƃ�ۏ؂��邽�߂Ɏg�p�����t�F���X�B
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &waitFences[i]));
		}
	}

	void createCommandBuffers()
	{
		// ���ׂẴR�}���h�o�b�t�@�́A�R�}���h�v�[�����犄�蓖�Ă��܂��B
		VkCommandPoolCreateInfo commandPoolCI{};
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCI.queueFamilyIndex = swapChain.queueNodeIndex;
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));

		// ��L�̃v�[������A�ő哯�����s�t���[�������Ƃ�1�̃R�}���h�o�b�t�@�����蓖�Ă܂��B
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_CONCURRENT_FRAMES);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, commandBuffers.data()));
	}

	// �C���f�b�N�X�t���O�p�`�̂��߂̒��_����уC���f�b�N�X�o�b�t�@���������܂��B
	// �܂��A�X�e�[�W���O���g�p���Ă������f�o�C�X���[�J���������ɃA�b�v���[�h���A���_�V�F�[�_�[�Ɉ�v����悤�ɒ��_���͂Ƒ����o�C���f�B���O�����������܂��B
	void createVertexBuffer()
	{
		// Vulkan�̃������Ǘ��S�ʂɊւ��钍�ӓ_�F
		//	����͔��ɕ��G�ȃg�s�b�N�ł���A�T���v���A�v���P�[�V�����ł͏����Ȍʂ̃������A���P�[�V�����Ŗ�肠��܂��񂪁A
		//	���ۂ̃A�v���P�[�V�����ōs���ׂ����Ƃł͂���܂���B���ۂ̃A�v���P�[�V�����ł́A��x�ɑ傫�ȃ������`�����N�����蓖�Ă�ׂ��ł��B

		// ���_�̐ݒ�
		std::vector<Vertex> vertexBuffer{
			{ { 1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
		};
		uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

		// �C���f�b�N�X�̐ݒ�
		std::vector<uint32_t> indexBuffer{ 0, 1, 2 };
		indices.count = static_cast<uint32_t>(indexBuffer.size());
		uint32_t indexBufferSize = indices.count * sizeof(uint32_t);

		VkMemoryAllocateInfo memAlloc{};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs;

		// ���_�o�b�t�@��C���f�b�N�X�o�b�t�@�̂悤�ȐÓI�f�[�^�́AGPU�ɂ��œK�i���ő��j�ȃA�N�Z�X�̂��߂Ƀf�o�C�X�������Ɋi�[�����ׂ��ł��B
		//
		// ������������邽�߂ɁA������u�X�e�[�W���O�o�b�t�@�v���g�p���܂��F
		// - �z�X�g������ȁi�����ă}�b�v�\�ȁj�o�b�t�@���쐬����
		// - �f�[�^�����̃o�b�t�@�ɃR�s�[����
		// - �f�o�C�X��ɓ����T�C�Y�̃��[�J���ȃo�b�t�@�iVRAM�j��������쐬����
		// - �R�}���h�o�b�t�@���g�p���ăz�X�g����f�o�C�X�փf�[�^���R�s�[����
		// - �z�X�g���́i�X�e�[�W���O�j�o�b�t�@���폜����
		// - �����_�����O�ɂ̓f�o�C�X���[�J���̃o�b�t�@���g�p����
		//
		// ���ӁF�z�X�g�iCPU�j��GPU�����������������L���铝���������A�[�L�e�N�`���ł́A�X�e�[�W���O�͕K�v����܂���B
		// ���̃T���v���𕪂���₷���ۂ��߁A���̃`�F�b�N�͍s���Ă��܂���B

		struct StagingBuffer {
			VkDeviceMemory memory;
			VkBuffer buffer;
		};

		struct {
			StagingBuffer vertices;
			StagingBuffer indices;
		} stagingBuffers;

		void* data;

		// ���_�o�b�t�@
		VkBufferCreateInfo vertexBufferInfoCI{};
		vertexBufferInfoCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfoCI.size = vertexBufferSize;
		// �o�b�t�@�̓R�s�[���Ƃ��Ďg�p����܂��B
		vertexBufferInfoCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// ���_�f�[�^���R�s�[���邽�߂̃z�X�g���o�b�t�@�i�X�e�[�W���O�o�b�t�@�j���쐬���܂��B
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &stagingBuffers.vertices.buffer));
		vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		// �f�[�^���R�s�[���邽�߂Ɏg�p�ł���z�X�g�� sicht�ȃ������^�C�v��v�����܂��B
		// �܂��A�o�b�t�@�̃A���}�b�v����ɏ������݂�GPU�Ɍ�����悤�ɁA�R�q�[�����g�ł��邱�Ƃ��v�����܂��B
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));
		// �}�b�v���ăR�s�[
		VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(device, stagingBuffers.vertices.memory);
		VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));

		// �i�z�X�g���[�J���ȁj���_�f�[�^���R�s�[����A�����_�����O�Ɏg�p�����f�o�C�X���[�J���ȃo�b�t�@���쐬���܂��B
		vertexBufferInfoCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &vertices.buffer));
		vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

		// �C���f�b�N�X�o�b�t�@
		VkBufferCreateInfo indexbufferCI{};
		indexbufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferCI.size = indexBufferSize;
		indexbufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// �C���f�b�N�X�f�[�^���z�X�g���̃o�b�t�@�i�X�e�[�W���O�o�b�t�@�j�ɃR�s�[���܂��B
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &stagingBuffers.indices.buffer));
		vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.indices.memory));
		VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &data));
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(device, stagingBuffers.indices.memory);
		VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0));

		// �f�o�C�X�̂݉��̈���o�b�t�@���쐬���܂��B
		indexbufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &indices.buffer));
		vkGetBufferMemoryRequirements(device, indices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &indices.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, indices.buffer, indices.memory, 0));

		// �o�b�t�@�R�s�[�̓L���[�ɃT�u�~�b�g����K�v�����邽�߁A���̂��߂̃R�}���h�o�b�t�@���K�v�ł��B
		// ���ӁF�ꕔ�̃f�o�C�X�́A��ʂ̃R�s�[���s���ۂɍ����ɂȂ�\���̂����p�̓]���L���[�i�]���r�b�g�݂̂��ݒ肳��Ă���j��񋟂��Ă��܂��B
		VkCommandBuffer copyCmd;

		VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = commandPool;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
		// �o�b�t�@�̈�̃R�s�[���R�}���h�o�b�t�@�ɓ���܂��B
		VkBufferCopy copyRegion{};
		// ���_�o�b�t�@
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vertices.buffer, 1, &copyRegion);
		// �C���f�b�N�X�o�b�t�@
		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffers.indices.buffer, indices.buffer, 1, &copyRegion);
		VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

		// �R�s�[���������邽�߂ɁA�R�}���h�o�b�t�@���L���[�ɃT�u�~�b�g���܂��B
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyCmd;

		// �R�}���h�o�b�t�@�̎��s�������������Ƃ�ۏ؂��邽�߂̃t�F���X���쐬���܂��B
		VkFenceCreateInfo fenceCI{};
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.flags = 0;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));

		// �L���[�ɃT�u�~�b�g���܂��B
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		// �t�F���X���R�}���h�o�b�t�@�̎��s������ʒm����̂�҂��܂��B
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

		vkDestroyFence(device, fence, nullptr);
		vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);

		// �X�e�[�W���O�o�b�t�@��j�����܂��B
		// ���ӁF�X�e�[�W���O�o�b�t�@�́A�R�s�[���T�u�~�b�g������s�����O�ɍ폜���Ă͂����܂���B
		vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
		vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
		vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
		vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);
	}

	// �f�B�X�N���v�^�̓v�[�����犄�蓖�Ă��܂��B���̃v�[���́A�g�p����f�B�X�N���v�^�̎�ނƁi�ő�j���������ɓ`���܂��B
	void createDescriptorPool()
	{
		// API�ɑ΂��āA�^���Ƃɗv�������f�B�X�N���v�^�̍ő吔��`����K�v������܂��B
		VkDescriptorPoolSize descriptorTypeCounts[1];
		// ���̃T���v���ł́A�f�B�X�N���v�^�̌^��1�i���j�t�H�[���o�b�t�@�j�����ł��B
		descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// �t���[�����Ƃ�1�̃o�b�t�@�i�܂�1�̃f�B�X�N���v�^�j������܂��B
		descriptorTypeCounts[0].descriptorCount = MAX_CONCURRENT_FRAMES;
		// ���̌^��ǉ�����ɂ́A�^�J�E���g���X�g�ɐV�����G���g����ǉ�����K�v������܂��B
		// ��F2�̌����C���[�W�T���v���[�̏ꍇ�F
		// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// typeCounts[1].descriptorCount = 2;

		// �O���[�o���ȃf�B�X�N���v�^�v�[�����쐬���܂��B
		// ���̃T���v���Ŏg�p����邷�ׂẴf�B�X�N���v�^�́A���̃v�[�����犄�蓖�Ă��܂��B
		VkDescriptorPoolCreateInfo descriptorPoolCI{};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.pNext = nullptr;
		descriptorPoolCI.poolSizeCount = 1;
		descriptorPoolCI.pPoolSizes = descriptorTypeCounts;
		// ���̃v�[������v���ł���f�B�X�N���v�^�Z�b�g�̍ő吔��ݒ肵�܂��i���̐����𒴂��ėv������ƃG���[�ɂȂ�܂��j�B
		// ���̃T���v���ł́A�t���[�����ƂɃ��j�t�H�[���o�b�t�@���Ƃ�1�̃Z�b�g���쐬���܂��B
		descriptorPoolCI.maxSets = MAX_CONCURRENT_FRAMES;
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));
	}

	// �f�B�X�N���v�^�Z�b�g���C�A�E�g�́A�A�v���P�[�V�����ƃV�F�[�_�[�Ԃ̃C���^�[�t�F�[�X���`���܂��B
	// ��{�I�ɁA�قȂ�V�F�[�_�[�X�e�[�W���A���j�t�H�[���o�b�t�@��C���[�W�T���v���[�Ȃǂ��o�C���h���邽�߂̃f�B�X�N���v�^�ɐڑ����܂��B
	// ���������āA���ׂẴV�F�[�_�[�o�C���f�B���O�́A1�̃f�B�X�N���v�^�Z�b�g���C�A�E�g�o�C���f�B���O�Ƀ}�b�s���O�����ׂ��ł��B
	void createDescriptorSetLayout()
	{
		// �o�C���f�B���O 0: ���j�t�H�[���o�b�t�@�i���_�V�F�[�_�[�j
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		layoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCI.pNext = nullptr;
		descriptorLayoutCI.bindingCount = 1;
		descriptorLayoutCI.pBindings = &layoutBinding;
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));

		// ���̃f�B�X�N���v�^�Z�b�g���C�A�E�g�Ɋ�Â��ă����_�����O�p�C�v���C���𐶐����邽�߂Ɏg�p�����p�C�v���C�����C�A�E�g���쐬���܂��B
		// ��蕡�G�ȃV�i���I�ł́A�ė��p�\�ȈقȂ�f�B�X�N���v�^�Z�b�g���C�A�E�g�ɑ΂��āA�قȂ�p�C�v���C�����C�A�E�g�������ƂɂȂ�܂��B
		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.pNext = nullptr;
		pipelineLayoutCI.setLayoutCount = 1;
		pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));
	}

	// �V�F�[�_�[�́A���j�t�H�[���o�b�t�@���u�w���v�f�B�X�N���v�^�Z�b�g���g�p���ăf�[�^�ɃA�N�Z�X���܂��B
	// �f�B�X�N���v�^�Z�b�g�́A��L�ō쐬�����f�B�X�N���v�^�Z�b�g���C�A�E�g�𗘗p���܂��B
	void createDescriptorSets()
	{
		// �O���[�o���ȃf�B�X�N���v�^�v�[������A�t���[�����Ƃ�1�̃f�B�X�N���v�^�Z�b�g�����蓖�Ă܂��B
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &descriptorSetLayout;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffers[i].descriptorSet));

			// �V�F�[�_�[�̃o�C���f�B���O�|�C���g�����肷��f�B�X�N���v�^�Z�b�g���X�V���܂��B
			// �V�F�[�_�[�Ŏg�p����邷�ׂẴo�C���f�B���O�|�C���g�ɑ΂��āA���̃o�C���f�B���O�|�C���g�Ɉ�v����f�B�X�N���v�^�Z�b�g��1�K�v�ł��B
			VkWriteDescriptorSet writeDescriptorSet{};

			// �o�b�t�@�̏��́A�f�B�X�N���v�^���\���̂��g�p���ēn����܂��B
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i].buffer;
			bufferInfo.range = sizeof(ShaderData);

			// �o�C���f�B���O 0 : ���j�t�H�[���o�b�t�@
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = uniformBuffers[i].descriptorSet;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.pBufferInfo = &bufferInfo;
			writeDescriptorSet.dstBinding = 0;
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	// �t���[���o�b�t�@�Ŏg�p�����[�x�i����уX�e���V���j�o�b�t�@�A�^�b�`�����g���쐬���܂��B
	// ���ӁF���N���X�̉��z�֐��̃I�[�o�[���C�h�ł���AVulkanExampleBase::prepare������Ăяo����܂��B
	void setupDepthStencil()
	{
		// �[�x�X�e���V���A�^�b�`�����g�Ƃ��Ďg�p�����œK�ȃC���[�W���쐬���܂��B
		VkImageCreateInfo imageCI{};
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = depthFormat;
		// �T���v���̕��ƍ������g�p���܂��B
		imageCI.extent = { width, height, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));

		// �C���[�W�̂��߂̃��������i�f�o�C�X���[�J���Ɂj���蓖�āA�������X�̃C���[�W�Ƀo�C���h���܂��B
		VkMemoryAllocateInfo memAlloc{};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &depthStencil.memory));
		VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0));

		// �[�x�X�e���V���C���[�W�̂��߂̃r���[���쐬���܂��B
		// Vulkan�ł̓C���[�W�͒��ڃA�N�Z�X���ꂸ�A�T�u���\�[�X�͈͂ɂ���ċL�q���ꂽ�r���[����ăA�N�Z�X����܂��B
		// ����ɂ��A�قȂ�͈͂�����1�̃C���[�W�̕����̃r���[���\�ɂȂ�܂��i��F�قȂ郌�C���[�̂��߁j�B
		VkImageViewCreateInfo depthStencilViewCI{};
		depthStencilViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthStencilViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilViewCI.format = depthFormat;
		depthStencilViewCI.subresourceRange = {};
		depthStencilViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		// �X�e���V���A�X�y�N�g�́A�[�x+�X�e���V���t�H�[�}�b�g�iVK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT�j�ł̂ݐݒ肷��K�v������܂��B
		if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
			depthStencilViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		depthStencilViewCI.subresourceRange.baseMipLevel = 0;
		depthStencilViewCI.subresourceRange.levelCount = 1;
		depthStencilViewCI.subresourceRange.baseArrayLayer = 0;
		depthStencilViewCI.subresourceRange.layerCount = 1;
		depthStencilViewCI.image = depthStencil.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilViewCI, nullptr, &depthStencil.view));
	}

	// �e�X���b�v�`�F�[���C���[�W�ɑ΂��ăt���[���o�b�t�@���쐬���܂��B
	// ���ӁF���N���X�̉��z�֐��̃I�[�o�[���C�h�ł���AVulkanExampleBase::prepare������Ăяo����܂��B
	void setupFrameBuffer()
	{
		// �X���b�v�`�F�[�����̂��ׂẴC���[�W�ɑ΂��ăt���[���o�b�t�@���쐬���܂��B
		frameBuffers.resize(swapChain.imageCount);
		for (size_t i = 0; i < frameBuffers.size(); i++)
		{
			std::array<VkImageView, 2> attachments;
			// �J���[�A�^�b�`�����g�̓X���b�v�`�F�[���C���[�W�̃r���[�ł��B
			attachments[0] = swapChain.buffers[i].view;
			// �[�x/�X�e���V���A�^�b�`�����g�́A���݂�GPU�ł̐[�x�̓�����@�̂��߁A���ׂẴt���[���o�b�t�@�œ����ł��B
			attachments[1] = depthStencil.view;

			VkFramebufferCreateInfo frameBufferCI{};
			frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			// ���ׂẴt���[���o�b�t�@�͓��������_�[�p�X�ݒ���g�p���܂��B
			frameBufferCI.renderPass = renderPass;
			frameBufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
			frameBufferCI.pAttachments = attachments.data();
			frameBufferCI.width = width;
			frameBufferCI.height = height;
			frameBufferCI.layers = 1;
			// �t���[���o�b�t�@���쐬���܂��B
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCI, nullptr, &frameBuffers[i]));
		}
	}

	// �����_�[�p�X�̐ݒ�
	// �����_�[�p�X��Vulkan�̐V�����T�O�ł��B�����_�����O���Ɏg�p�����A�^�b�`�����g���L�q���A�A�^�b�`�����g�̈ˑ��֌W���������̃T�u�p�X���܂ނ��Ƃ��ł��܂��B
	// ����ɂ��A�h���C�o�[�̓����_�����O���ǂ̂悤�ɂȂ邩�����O�ɒm�邱�Ƃ��ł��A���Ƀ^�C���x�[�X�̃����_���[�i�����̃T�u�p�X�����j�ł̍œK���̗ǂ��@��ƂȂ�܂��B
	// �T�u�p�X�̈ˑ��֌W���g�p����ƁA�g�p�����A�^�b�`�����g�̈ÖٓI�ȃ��C�A�E�g�J�ڂ��ǉ�����邽�߁A������ϊ����邽�߂̖����I�ȃC���[�W�������o���A��ǉ�����K�v�͂���܂���B
	// ���ӁF���N���X�̉��z�֐��̃I�[�o�[���C�h�ł���AVulkanExampleBase::prepare������Ăяo����܂��B
	void setupRenderPass()
	{
		// ���̃T���v���ł́A1�̃T�u�p�X�����P��̃����_�[�p�X���g�p���܂��B

		// ���̃����_�[�p�X�Ŏg�p�����A�^�b�`�����g�̋L�q�q�B
		std::array<VkAttachmentDescription, 2> attachments{};

		// �J���[�A�^�b�`�����g
		attachments[0].format = swapChain.colorFormat;                  // �X���b�v�`�F�[���őI�����ꂽ�J���[�t�H�[�}�b�g���g�p���܂��B
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;                  // ���̃T���v���ł̓}���`�T���v�����O�͎g�p���܂���B
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;             // �����_�[�p�X�̊J�n���ɂ��̃A�^�b�`�����g���N���A���܂��B
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;           // �����_�[�p�X�I��������̓��e��ێ����܂��i�\���̂��߁j�B
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // �X�e���V���͎g�p���Ȃ��̂ŁA���[�h�͋C�ɂ��܂���B
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // �X�g�A�����l�ł��B
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;        // �����_�[�p�X�J�n���̃��C�A�E�g�B�������C�A�E�g�͏d�v�ł͂Ȃ��̂ŁAundefined���g�p���܂��B
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;    // �����_�[�p�X�I�����ɃA�^�b�`�����g���J�ڂ��郌�C�A�E�g�B
		// �J���[�o�b�t�@���X���b�v�`�F�[���ɒ񎦂��������߁APRESENT_KHR�ɑJ�ڂ��܂��B
// �[�x�A�^�b�`�����g
		attachments[1].format = depthFormat;                           // �K�؂Ȑ[�x�t�H�[�}�b�g���T���v�����N���X�őI������܂��B
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;             // �ŏ��̃T�u�p�X�̊J�n���ɐ[�x���N���A���܂��B
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;       // �����_�[�p�X�I����ɐ[�x�͕s�v�ł��iDONT_CARE�̓p�t�H�[�}���X����ɂȂ���\��������܂��j�B
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // �X�e���V���Ȃ��B
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // �X�e���V���Ȃ��B
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;        // �����_�[�p�X�J�n���̃��C�A�E�g�B�������C�A�E�g�͏d�v�ł͂Ȃ��̂ŁAundefined���g�p���܂��B
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // �[�x/�X�e���V���A�^�b�`�����g�ɑJ�ڂ��܂��B

		// �A�^�b�`�����g�Q�Ƃ̐ݒ�
		VkAttachmentReference colorReference{};
		colorReference.attachment = 0;                                   // �A�^�b�`�����g0�̓J���[�ł��B
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // �T�u�p�X���ɃJ���[�Ƃ��Ďg�p�����A�^�b�`�����g���C�A�E�g�B

		VkAttachmentReference depthReference{};
		depthReference.attachment = 1;                                     // �A�^�b�`�����g1�͐[�x�ł��B
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // �T�u�p�X���ɐ[�x/�X�e���V���Ƃ��Ďg�p�����A�^�b�`�����g�B

		// �P��̃T�u�p�X�Q�Ƃ̐ݒ�
		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;                     // �T�u�p�X��1�̃J���[�A�^�b�`�����g���g�p���܂��B
		subpassDescription.pColorAttachments = &colorReference;          // �X���b�g0�̃J���[�A�^�b�`�����g�ւ̎Q�ƁB
		subpassDescription.pDepthStencilAttachment = &depthReference;    // �X���b�g1�̐[�x�A�^�b�`�����g�ւ̎Q�ƁB
		subpassDescription.inputAttachmentCount = 0;                     // ���̓A�^�b�`�����g�́A�O�̃T�u�p�X�̓��e����T���v�����O���邽�߂Ɏg�p�ł��܂��B
		subpassDescription.pInputAttachments = nullptr;                  // (���̃T���v���ł͓��̓A�^�b�`�����g�͎g�p���܂���)
		subpassDescription.preserveAttachmentCount = 0;                  // �ێ��A�^�b�`�����g�́A�T�u�p�X�ԂŃA�^�b�`�����g�����[�v�i����ѕێ��j���邽�߂Ɏg�p�ł��܂��B
		subpassDescription.pPreserveAttachments = nullptr;               // (���̃T���v���ł͕ێ��A�^�b�`�����g�͎g�p���܂���)
		subpassDescription.pResolveAttachments = nullptr;                // �����A�^�b�`�����g�̓T�u�p�X�̍Ō�ɉ�������A�}���`�T���v�����O�ȂǂɎg�p�ł��܂��B

		// �T�u�p�X�ˑ��֌W�̐ݒ�
		// �����́A�A�^�b�`�����g�L�q�Ŏw�肳�ꂽ�ÖٓI�ȃA�^�b�`�����g���C�A�E�g�J�ڂ�ǉ����܂��B
		// ���ۂ̎g�p���C�A�E�g�́A�A�^�b�`�����g�Q�ƂŎw�肳�ꂽ���C�A�E�g��ʂ��ĕێ�����܂��B
		// �e�T�u�p�X�ˑ��֌W�́AsrcStageMask, dstStageMask, srcAccessMask, dstAccessMask�ɂ���ċL�q�����\�[�X�T�u�p�X�ƃf�X�e�B�l�[�V�����T�u�p�X�̊ԂɃ���������ю��s�̈ˑ��֌W�𓱓����܂��i������dependencyFlags���ݒ肳��܂��j�B
		// ���ӁFVK_SUBPASS_EXTERNAL�́A���ۂ̃����_�[�p�X�̊O���Ŏ��s����邷�ׂẴR�}���h���Q�Ƃ�����ʂȒ萔�ł��B
		std::array<VkSubpassDependency, 2> dependencies;

		// �[�x����уJ���[�A�^�b�`�����g��final����initial�ւ̃��C�A�E�g�J�ڂ��s���܂��B
		// �[�x�A�^�b�`�����g
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dependencies[0].dependencyFlags = 0;
		// �J���[�A�^�b�`�����g
		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = 0;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[1].dependencyFlags = 0;

		// ���ۂ̃����_�[�p�X���쐬���܂��B
		VkRenderPassCreateInfo renderPassCI{};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size()); // ���̃����_�[�p�X�Ŏg�p�����A�^�b�`�����g�̐��B
		renderPassCI.pAttachments = attachments.data();                           // �����_�[�p�X�Ŏg�p�����A�^�b�`�����g�̋L�q�B
		renderPassCI.subpassCount = 1;                                           // ���̃T���v���ł�1�̃T�u�p�X�̂ݎg�p���܂��B
		renderPassCI.pSubpasses = &subpassDescription;                           // ���̃T�u�p�X�̋L�q�B
		renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size()); // �T�u�p�X�ˑ��֌W�̐��B
		renderPassCI.pDependencies = dependencies.data();                         // �����_�[�p�X�Ŏg�p�����T�u�p�X�ˑ��֌W�B
		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderPass));
	}

	// Vulkan�́ASPIR-V�ƌĂ΂�钆�ԃo�C�i���\������V�F�[�_�[�����[�h���܂��B
	// �V�F�[�_�[�́A�Ⴆ��GLSL����Q��glslang�R���p�C�����g�p���ăI�t���C���ŃR���p�C������܂��B
	// ���̊֐��́A���̂悤�ȃV�F�[�_�[���o�C�i���t�@�C�����烍�[�h���A�V�F�[�_�[���W���[���\���̂�Ԃ��܂��B
	VkShaderModule loadSPIRVShader(std::string filename)
	{
		size_t shaderSize;
		char* shaderCode{ nullptr };

#if defined(__ANDROID__)
		// ���k���ꂽ�A�Z�b�g����V�F�[�_�[�����[�h���܂��B
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
		assert(asset);
		shaderSize = AAsset_getLength(asset);
		assert(shaderSize > 0);

		shaderCode = new char[shaderSize];
		AAsset_read(asset, shaderCode, shaderSize);
		AAsset_close(asset);
#else
		std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

		if (is.is_open())
		{
			shaderSize = is.tellg();
			is.seekg(0, std::ios::beg);
			// �t�@�C���̓��e���o�b�t�@�ɃR�s�[���܂��B
			shaderCode = new char[shaderSize];
			is.read(shaderCode, shaderSize);
			is.close();
			assert(shaderSize > 0);
		}
#endif
		if (shaderCode)
		{
			// �p�C�v���C���쐬�Ɏg�p�����V�����V�F�[�_�[���W���[�����쐬���܂��B
			VkShaderModuleCreateInfo shaderModuleCI{};
			shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleCI.codeSize = shaderSize;
			shaderModuleCI.pCode = (uint32_t*)shaderCode;

			VkShaderModule shaderModule;
			VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));

			delete[] shaderCode;

			return shaderModule;
		}
		else
		{
			std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
			return VK_NULL_HANDLE;
		}
	}

	void createPipelines()
	{
		// ���̃T���v���Ŏg�p�����O���t�B�b�N�X�p�C�v���C�����쐬���܂��B
		// Vulkan�̓����_�����O�p�C�v���C���̊T�O���g�p���ČŒ�X�e�[�g���J�v�Z�������AOpenGL�̕��G�ȃX�e�[�g�}�V����u�������܂��B
		// �p�C�v���C����GPU��Ɋi�[����n�b�V��������邽�߁A�p�C�v���C���̕ύX�͔��ɍ����ł��B
		// ���ӁF�p�C�v���C���ɒ��ڊ܂܂�Ȃ����I�ȃX�e�[�g���������܂����݂��܂��i�������A����炪�g�p�����Ƃ������͊܂܂�܂��j�B

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		// ���̃p�C�v���C���Ɏg�p����郌�C�A�E�g�i�������C�A�E�g���g�p���镡���̃p�C�v���C���Ԃŋ��L�\�j�B
		pipelineCI.layout = pipelineLayout;
		// ���̃p�C�v���C�����A�^�b�`����郌���_�[�p�X�B
		pipelineCI.renderPass = renderPass;

		// �p�C�v���C�����\�����邳�܂��܂ȃX�e�[�g���\�z���܂��B

		// ���̓A�Z���u���X�e�[�g�́A�v���~�e�B�u���ǂ̂悤�ɑg�ݗ��Ă��邩���L�q���܂��B
		// ���̃p�C�v���C���͒��_�f�[�^���g���C�A���O�����X�g�Ƃ��đg�ݗ��Ă܂��i�������A1�̃g���C�A���O�������g�p���܂���j�B
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// ���X�^���C�[�[�V�����X�e�[�g
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCI.depthClampEnable = VK_FALSE;
		rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateCI.depthBiasEnable = VK_FALSE;
		rasterizationStateCI.lineWidth = 1.0f;

		// �J���[�u�����h�X�e�[�g�́A�i�g�p����Ă���ꍇ�j�u�����h�W�����ǂ̂悤�Ɍv�Z����邩���L�q���܂��B
		// �i�u�����f�B���O���g�p����Ă��Ȃ��Ă��j�J���[�A�^�b�`�����g���Ƃ�1�̃u�����h�A�^�b�`�����g�X�e�[�g���K�v�ł��B
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = 0xf;
		blendAttachmentState.blendEnable = VK_FALSE;
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentState;

		// �r���[�|�[�g�X�e�[�g�́A���̃p�C�v���C���Ŏg�p�����r���[�|�[�g�ƃV�U�[�̐���ݒ肵�܂��B
		// ���ӁF����͎��ۂɂ͓��I�X�e�[�g�ɂ���ď㏑������܂��i���L�Q�Ɓj�B
		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;

		// ���I�X�e�[�g�̗L����
		// �قƂ�ǂ̃X�e�[�g�̓p�C�v���C���ɏĂ��t�����܂����A�R�}���h�o�b�t�@���ŕύX�ł��铮�I�ȃX�e�[�g������������܂��B
		// ������ύX�ł���悤�ɂ���ɂ́A���̃p�C�v���C���łǂ̓��I�X�e�[�g���ύX����邩���w�肷��K�v������܂��B���ۂ̃X�e�[�g�͌�ŃR�}���h�o�b�t�@�Őݒ肳��܂��B
		// ���̃T���v���ł́A�r���[�|�[�g�ƃV�U�[�𓮓I�X�e�[�g���g�p���Đݒ肵�܂��B
		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// �[�x����уX�e���V���̔�r�ƃe�X�g������܂ށA�[�x����уX�e���V���X�e�[�g�B
		// �[�x�e�X�g�݂̂��g�p���A�[�x�e�X�g�Ə������݂�L���ɂ��Aless or equal�Ŕ�r���܂��B
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_TRUE;
		depthStencilStateCI.depthWriteEnable = VK_TRUE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
		depthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilStateCI.stencilTestEnable = VK_FALSE;
		depthStencilStateCI.front = depthStencilStateCI.back;

		// �}���`�T���v�����O�X�e�[�g
		// ���̃T���v���ł̓}���`�T���v�����O�i�A���`�G�C���A�V���O�p�j���g�p���܂��񂪁A�X�e�[�g�͐ݒ肵�ăp�C�v���C���ɓn���K�v������܂��B
		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleStateCI.pSampleMask = nullptr;

		// ���_���͂̋L�q
		// �p�C�v���C���̒��_���̓p�����[�^���w�肵�܂��B

		// ���_���̓o�C���f�B���O
		// ���̃T���v���ł́A�o�C���f�B���O�|�C���g0�ŒP��̒��_���̓o�C���f�B���O���g�p���܂��ivkCmdBindVertexBuffers���Q�Ɓj�B
		VkVertexInputBindingDescription vertexInputBinding{};
		vertexInputBinding.binding = 0;
		vertexInputBinding.stride = sizeof(Vertex);
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// ���͑����o�C���f�B���O�́A�V�F�[�_�[�����̈ʒu�ƃ��������C�A�E�g���L�q���܂��B
		std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs;
		// �����͎��̃V�F�[�_�[���C�A�E�g�Ɉ�v���܂��itriangle.vert���Q�Ɓj�F
		//	layout (location = 0) in vec3 inPos;
		//	layout (location = 1) in vec3 inColor;
		// �����ʒu 0: �ʒu
		vertexInputAttributs[0].binding = 0;
		vertexInputAttributs[0].location = 0;
		// �ʒu������3��32�r�b�g�����t�����������_���iSFLOAT�j�ł��iR32 G32 B32�j�B
		vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributs[0].offset = offsetof(Vertex, position);
		// �����ʒu 1: �F
		vertexInputAttributs[1].binding = 0;
		vertexInputAttributs[1].location = 1;
		// �F������3��32�r�b�g�����t�����������_���iSFLOAT�j�ł��iR32 G32 B32�j�B
		vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributs[1].offset = offsetof(Vertex, color);

		// �p�C�v���C���쐬�Ɏg�p����钸�_���̓X�e�[�g�B
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
		vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCI.vertexBindingDescriptionCount = 1;
		vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
		vertexInputStateCI.vertexAttributeDescriptionCount = 2;
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributs.data();

		// �V�F�[�_�[
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		// ���_�V�F�[�_�[
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// ���̃V�F�[�_�[�̃p�C�v���C���X�e�[�W��ݒ肵�܂��B
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		// �o�C�i����SPIR-V�V�F�[�_�[�����[�h���܂��B
		shaderStages[0].module = loadSPIRVShader(getShadersPath() + "triangle/triangle.vert.spv");
		// �V�F�[�_�[�̃��C���G���g���[�|�C���g�B
		shaderStages[0].pName = "main";
		assert(shaderStages[0].module != VK_NULL_HANDLE);

		// �t���O�����g�V�F�[�_�[
		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// ���̃V�F�[�_�[�̃p�C�v���C���X�e�[�W��ݒ肵�܂��B
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		// �o�C�i����SPIR-V�V�F�[�_�[�����[�h���܂��B
		shaderStages[1].module = loadSPIRVShader(getShadersPath() + "triangle/triangle.frag.spv");
		// �V�F�[�_�[�̃��C���G���g���[�|�C���g�B
		shaderStages[1].pName = "main";
		assert(shaderStages[1].module != VK_NULL_HANDLE);

		// �p�C�v���C���V�F�[�_�[�X�e�[�W����ݒ肵�܂��B
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		// �p�C�v���C���X�e�[�g���p�C�v���C���쐬���\���̂Ɋ��蓖�Ă܂��B
		pipelineCI.pVertexInputState = &vertexInputStateCI;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;

		// �w�肳�ꂽ�X�e�[�g���g�p���ă����_�����O�p�C�v���C�����쐬���܂��B
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

		// �O���t�B�b�N�X�p�C�v���C�����쐬�����ƁA�V�F�[�_�[���W���[���͕s�v�ɂȂ�܂��B
		vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
		vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
	}

	void createUniformBuffers()
	{
		// �V�F�[�_�[���j�t�H�[�����܂ރt���[�����Ƃ̃��j�t�H�[���o�b�t�@�u���b�N�������E���������܂��B
		// OpenGL�̂悤�ȒP��̃��j�t�H�[����Vulkan�ɂ͂��͂⑶�݂��܂���B���ׂẴV�F�[�_�[���j�t�H�[���̓��j�t�H�[���o�b�t�@�u���b�N����ēn����܂��B
		VkMemoryRequirements memReqs;

		// ���_�V�F�[�_�[�̃��j�t�H�[���o�b�t�@�u���b�N
		VkBufferCreateInfo bufferInfo{};
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = 0;
		allocInfo.memoryTypeIndex = 0;

		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(ShaderData);
		// ���̃o�b�t�@�̓��j�t�H�[���o�b�t�@�Ƃ��Ďg�p����܂��B
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		// �o�b�t�@���쐬���܂��B
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffers[i].buffer));
			// �T�C�Y�A�A���C�����g�A�������^�C�v���܂ރ������v�����擾���܂��B
			vkGetBufferMemoryRequirements(device, uniformBuffers[i].buffer, &memReqs);
			allocInfo.allocationSize = memReqs.size;
			// �z�X�g���̃������A�N�Z�X���T�|�[�g���郁�����^�C�v�̃C���f�b�N�X���擾���܂��B
			// �قƂ�ǂ̎����͕����̃������^�C�v��񋟂��Ă���A�����������蓖�Ă邽�߂ɐ��������̂�I�����邱�Ƃ��d�v�ł��B
			// �܂��A�o�b�t�@���z�X�g�R�q�[�����g�ł��邱�Ƃ�]�݂܂��B��������΁A�X�V�̂��тɃt���b�V���i�܂��͓����j����K�v������܂���B
			// ���ӁF����̓p�t�H�[�}���X�ɉe����^����\�������邽�߁A����I�Ƀo�b�t�@���X�V������ۂ̃A�v���P�[�V�����ł͍s�������Ȃ���������܂���B
			allocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			// ���j�t�H�[���o�b�t�@�̂��߂̃����������蓖�Ă܂��B
			VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &(uniformBuffers[i].memory)));
			// ���������o�b�t�@�Ƀo�C���h���܂��B
			VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffers[i].buffer, uniformBuffers[i].memory, 0));
			// �o�b�t�@����x�}�b�v���Ă������ƂŁA�ēx�}�b�v���邱�ƂȂ��X�V�ł��܂��B
			VK_CHECK_RESULT(vkMapMemory(device, uniformBuffers[i].memory, 0, sizeof(ShaderData), 0, (void**)&uniformBuffers[i].mapped));
		}

	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		createSynchronizationPrimitives();
		createCommandBuffers();
		createVertexBuffer();
		createUniformBuffers();
		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSets();
		createPipelines();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;

		// �t�F���X���g�p���āA�R�}���h�o�b�t�@���ēx�g�p����O�ɂ��̎��s����������̂�҂��܂��B
		vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX);
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentFrame]));

		// �������玟�̃X���b�v�`�F�[���C���[�W���擾���܂��B
		// �����͔C�ӂ̏����ŃC���[�W��Ԃ����Ƃ��ł��邽�߁Aacquire�֐����g�p����K�v������A�P�ɃC���[�W/imageIndex�������Ń��[�v���邱�Ƃ͂ł��܂���B
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			windowResize();
			return;
		}
		else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)) {
			throw "Could not acquire the next swap chain image!";
		}

		// ���̃t���[���̂��߂Ƀ��j�t�H�[���o�b�t�@���X�V���܂��B
		ShaderData shaderData{};
		shaderData.projectionMatrix = camera.matrices.perspective;
		shaderData.viewMatrix = camera.matrices.view;
		shaderData.modelMatrix = glm::mat4(1.0f);

		// ���݂̍s������݂̃t���[���̃��j�t�H�[���o�b�t�@�ɃR�s�[���܂��B
		// ���ӁF���j�t�H�[���o�b�t�@�Ƀz�X�g�R�q�[�����g�ȃ������^�C�v��v���������߁A�������݂͑�����GPU�ɉ��ɂȂ�܂��B
		memcpy(uniformBuffers[currentFrame].mapped, &shaderData, sizeof(ShaderData));

		// �R�}���h�o�b�t�@���\�z���܂��B
		// OpenGL�Ƃ͈قȂ�A���ׂẴ����_�����O�R�}���h�̓R�}���h�o�b�t�@�ɋL�^����A���̌�L���[�ɃT�u�~�b�g����܂��B
		// ����ɂ��A�ʂ̃X���b�h�Ŏ��O�ɍ�Ƃ𐶐��ł��܂��B
		// �i���̃T���v���̂悤�ȁj��{�I�ȃR�}���h�o�b�t�@�ł́A�L�^�����ɍ����Ȃ��߁A������I�t���[�h����K�v�͂���܂���B

		vkResetCommandBuffer(commandBuffers[currentFrame], 0);

		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		// loadOp��clear�ɐݒ肳��Ă��邷�ׂẴt���[���o�b�t�@�A�^�b�`�����g�̃N���A�l��ݒ肵�܂��B
		// 2�̃A�^�b�`�����g�i�J���[�Ɛ[�x�j���g�p���A�����̓T�u�p�X�̊J�n���ɃN���A����邽�߁A�����̃N���A�l��ݒ肷��K�v������܂��B
		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = frameBuffers[imageIndex];

		const VkCommandBuffer commandBuffer = commandBuffers[currentFrame];
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

		// ���N���X�ɂ���ăf�t�H���g�̃����_�[�p�X�ݒ�Ŏw�肳�ꂽ�ŏ��̃T�u�p�X���J�n���܂��B
		// ����ɂ��A�J���[�Ɛ[�x�̃A�^�b�`�����g���N���A����܂��B
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		// ���I�ȃr���[�|�[�g�X�e�[�g���X�V���܂��B
		VkViewport viewport{};
		viewport.height = (float)height;
		viewport.width = (float)width;
		viewport.minDepth = (float)0.0f;
		viewport.maxDepth = (float)1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		// ���I�ȃV�U�[�X�e�[�g���X�V���܂��B
		VkRect2D scissor{};
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		// ���݂̃t���[���̃��j�t�H�[���o�b�t�@�̃f�B�X�N���v�^�Z�b�g���o�C���h���A���̕`��ŃV�F�[�_�[�����̃o�b�t�@�̃f�[�^���g�p����悤�ɂ��܂��B
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uniformBuffers[currentFrame].descriptorSet, 0, nullptr);
		// �����_�����O�p�C�v���C�����o�C���h���܂��B
		// �p�C�v���C���i�X�e�[�g�I�u�W�F�N�g�j�ɂ̓����_�����O�p�C�v���C���̂��ׂẴX�e�[�g���܂܂�Ă���A������o�C���h����ƃp�C�v���C���쐬���Ɏw�肳�ꂽ���ׂẴX�e�[�g���ݒ肳��܂��B
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// �O�p�`�̒��_�o�b�t�@�i�ʒu�ƐF���܂ށj���o�C���h���܂��B
		VkDeviceSize offsets[1]{ 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
		// �O�p�`�̃C���f�b�N�X�o�b�t�@���o�C���h���܂��B
		vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		// �C���f�b�N�X�t���O�p�`��`�悵�܂��B
		vkCmdDrawIndexed(commandBuffer, indices.count, 1, 0, 0, 1);
		vkCmdEndRenderPass(commandBuffer);
		// �����_�[�p�X���I������ƁA�t���[���o�b�t�@�̃J���[�A�^�b�`�����g���E�B���h�E�V�X�e���ɒ񎦂��邽�߂�VK_IMAGE_LAYOUT_PRESENT_SRC_KHR�ɑJ�ڂ�����Öق̃o���A���ǉ�����܂��B
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		// �R�}���h�o�b�t�@���O���t�B�b�N�X�L���[�ɃT�u�~�b�g���܂��B

		// �L���[�̃T�u�~�b�V�������ipWaitSemaphores����āj�ҋ@����p�C�v���C���X�e�[�W�B
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		// submit info�\���̂́A�R�}���h�o�b�t�@�̃L���[�T�u�~�b�V�����o�b�`���w�肵�܂��B
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &waitStageMask;       // �Z�}�t�H�̑ҋ@����������p�C�v���C���X�e�[�W�̃��X�g�ւ̃|�C���^�B
		submitInfo.pCommandBuffers = &commandBuffer;		// ���̃o�b�`�i�T�u�~�b�V�����j�Ŏ��s����R�}���h�o�b�t�@�B
		submitInfo.commandBufferCount = 1;                   // �P��̃R�}���h�o�b�t�@���T�u�~�b�g���܂��B

		// �T�u�~�b�g���ꂽ�R�}���h�o�b�t�@�����s���J�n����O�ɑҋ@����Z�}�t�H�B
		submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentFrame];
		submitInfo.waitSemaphoreCount = 1;
		// �R�}���h�o�b�t�@�����������Ƃ��ɃV�O�i�������Z�}�t�H�B
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentFrame];
		submitInfo.signalSemaphoreCount = 1;

		// �ҋ@�t�F���X��n���ăO���t�B�b�N�X�L���[�ɃT�u�~�b�g���܂��B
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentFrame]));

		// ���݂̃t���[���o�b�t�@���X���b�v�`�F�[���ɒ񎦂��܂��B
		// �R�}���h�o�b�t�@�̃T�u�~�b�V�����ɂ���ăV�O�i�����ꂽ�Z�}�t�H���A�X���b�v�`�F�[���񎦂̑ҋ@�Z�}�t�H�Ƃ��ēn���܂��B
		// ����ɂ��A���ׂẴR�}���h���T�u�~�b�g�����܂ŁA�C���[�W���E�B���h�E�V�X�e���ɒ񎦂���Ȃ����Ƃ��ۏ؂���܂��B

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphores[currentFrame];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain.swapChain;
		presentInfo.pImageIndices = &imageIndex;
		result = vkQueuePresentKHR(queue, &presentInfo);

		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
			windowResize();
		}
		else if (result != VK_SUCCESS) {
			throw "Could not present the image to the swap chain!";
		}

		// �ő哯�����s�t���[�����Ɋ�Â��āA���Ƀ����_�����O����t���[����I�����܂��B
		currentFrame = (currentFrame + 1) % MAX_CONCURRENT_FRAMES;
	}
};

// OS�ŗL�̃��C���G���g���[�|�C���g
// �R�[�h�x�[�X�̂قƂ�ǂ́A�T�|�[�g����Ă��邳�܂��܂ȃI�y���[�e�B���O�V�X�e���ŋ��L����Ă��܂����A���b�Z�[�W�����Ȃǂ͈قȂ�܂��B

#if defined(_WIN32)
// Windows�̃G���g���[�|�C���g
VulkanExample* vulkanExample;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
	for (size_t i = 0; i < __argc; i++) { VulkanExample::args.push_back(__argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow(hInstance, WndProc);
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}

#elif defined(__ANDROID__)
// Android�̃G���g���[�|�C���g
VulkanExample* vulkanExample;
void android_main(android_app* state)
{
	vulkanExample = new VulkanExample();
	state->userData = vulkanExample;
	state->onAppCmd = VulkanExample::handleAppCommand;
	state->onInputEvent = VulkanExample::handleAppInput;
	androidApp = state;
	vulkanExample->renderLoop();
	delete(vulkanExample);
}
#elif defined(_DIRECT2DISPLAY)

// Direct to display wsi ���g�p���� Linux �̃G���g���[�|�C���g
// Direct to Displays (D2D) �͑g�ݍ��݃v���b�g�t�H�[���Ŏg�p����܂��B
VulkanExample* vulkanExample;
static void handleEvent()
{
}
int main(const int argc, const char* argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
VulkanExample* vulkanExample;
static void handleEvent(const DFBWindowEvent* event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
int main(const int argc, const char* argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
VulkanExample* vulkanExample;
int main(const int argc, const char* argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(__linux__) || defined(__FreeBSD__)

// Linux�̃G���g���[�|�C���g
VulkanExample* vulkanExample;
#if defined(VK_USE_PLATFORM_XCB_KHR)
static void handleEvent(const xcb_generic_event_t* event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
#else
static void handleEvent()
{
}
#endif
int main(const int argc, const char* argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif (defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)) && defined(VK_EXAMPLE_XCODE_GENERATED)
VulkanExample* vulkanExample;
int main(const int argc, const char* argv[])
{
	@autoreleasepool
	{
		for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
		vulkanExample = new VulkanExample();
		vulkanExample->initVulkan();
		vulkanExample->setupWindow(nullptr);
		vulkanExample->prepare();
		vulkanExample->renderLoop();
		delete(vulkanExample);
	}
	return 0;
}
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
VULKAN_EXAMPLE_MAIN()
#endif