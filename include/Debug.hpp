#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>

#if !defined(__ANDROID__)
#include <GLFW/glfw3.h>
#endif

namespace Vulkan_Test
{
    void debugApplicationInfo(vk::ApplicationInfo &applicationInfo)
    {
        LOG("----------------------------------------");
        LOG("Debug ApplicationInfo");

        LOG("apiVersion: " << 
            VK_VERSION_MAJOR(applicationInfo.apiVersion) << "." << 
            VK_VERSION_MINOR(applicationInfo.apiVersion) << "." <<
            VK_VERSION_PATCH(applicationInfo.apiVersion));
    }

    void debugInstanceCreateInfo(vk::InstanceCreateInfo &instanceCreateInfo)
    {
        LOG("----------------------------------------");
        LOG("Debug Instance Extensions");
        LOG("enabledExtensionCount: " << instanceCreateInfo.enabledExtensionCount);
        SET_LOG_INDEX(1);
        for (int i = 0; i < instanceCreateInfo.enabledExtensionCount; i++)
        {
            LOG("----------------------------------------");
            LOG("    " << instanceCreateInfo.ppEnabledExtensionNames[i]);
        }
        SET_LOG_INDEX(0);
    }

#if !defined(__ANDROID__)
    void debugGlfwWindow(GLFWwindow &window)
    {
        LOG("----------------------------------------");
        LOG("Debug GLFW Window");
        int width;
        int height;
        glfwGetWindowSize(&window, &width, &height);
        LOG("WindowSize: width " << width << ", height " << height);
        glfwGetFramebufferSize(&window, &width, &height);
        LOG("FramebufferSize: width " << width << ", height " << height);
    }
#endif

    // UUIDを16進数で表示する関数
    std::string getUUID(const uint8_t* uuid, size_t size)
    {
        std::stringstream ss;
        for (size_t i = 0; i < size; ++i) 
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(uuid[i]);
            if (i < size - 1) 
            {
                ss << ":";
            }
        }
        return ss.str();
    }

    void debugPhysicalDevices(std::vector<vk::PhysicalDevice> &physicalDevices)
    {
        LOG("----------------------------------------");
        LOG("Debug Physical Devices");
        LOG("physicalDevicesCount: " << physicalDevices.size());
        SET_LOG_INDEX(1);
        for (vk::PhysicalDevice physicalDevice : physicalDevices)
        {
            LOG("----------------------------------------");
            vk::PhysicalDeviceProperties2 props;
            vk::PhysicalDeviceVulkan12Properties props12;
            vk::PhysicalDeviceVulkan11Properties props11;
            props.pNext = &props12;
            props12.pNext = &props11;
            physicalDevice.getProperties2(&props);

            LOG(props.properties.deviceName);
            LOG("deviceID: " << props.properties.deviceID);
            LOG("vendorID: " << props.properties.vendorID);
            LOG("deviceUUID: " << getUUID(props11.deviceUUID, VK_UUID_SIZE));
            LOG("driverName: " << props12.driverName);
            LOG("driverInfo: " << props12.driverInfo);
            LOG("maxMemoryAllocationSize: " << static_cast<unsigned long long>(props11.maxMemoryAllocationSize));
        }
        SET_LOG_INDEX(0);
    }

    void debugPhysicalDevice(vk::PhysicalDevice &physicalDevice, uint32_t queueFamilyIndex)
    {
        LOG("----------------------------------------");
        LOG("Debug Physical Devices");

        vk::PhysicalDeviceProperties2 props = physicalDevice.getProperties2();
        LOG("Found device and queue");
        LOG("physicalDevice: " << props.properties.deviceName);
        LOG("queueFamilyIndex: " << queueFamilyIndex);
    }

    void debugPhysicalMemory(vk::PhysicalDevice &physicalDevice)
    {
        LOG("----------------------------------------");
        LOG("Debug Physical Memory");
        vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();
        LOG("memory type count: " << memProps.memoryTypeCount);
        LOG("memory heap count: " << memProps.memoryHeapCount);
        SET_LOG_INDEX(1);
        for (size_t i = 0; i < memProps.memoryTypeCount; i++)
        {
            LOG("----------------------------------------");
            LOG("memory index: " << i);
            LOG(to_string(memProps.memoryTypes[i].propertyFlags));
        }
        SET_LOG_INDEX(0);
    }

    void debugQueueFamilyProperties(std::vector<vk::QueueFamilyProperties> &queueFamilyProperties)
    {
        LOG("----------------------------------------");
        LOG("Debug Queue Family Properties");
        LOG("queue family count: " << queueFamilyProperties.size());
        SET_LOG_INDEX(1);
        for (size_t i = 0; i < queueFamilyProperties.size(); i++)
        {
            LOG("----------------------------------------");
            LOG("queue family index: " << i);
            LOG("queue count: " << queueFamilyProperties[i].queueCount);
            LOG(to_string(queueFamilyProperties[i].queueFlags));
        }
        SET_LOG_INDEX(0);
    }

    void debugSwapchainCreateInfo(vk::SwapchainCreateInfoKHR& swapchainCreateInfo)
    {
        LOG("----------------------------------------");
        LOG("Debug Swapchain Create Info");
        LOG("swapchain minImageCount " << swapchainCreateInfo.minImageCount);
        LOG("swapchainCreateInfo " << swapchainCreateInfo.imageExtent.width << ", " << swapchainCreateInfo.imageExtent.height);
    }
}
