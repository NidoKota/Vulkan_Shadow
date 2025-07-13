#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

std::shared_ptr<GLFWwindow> getGlfwWindow(int width, int height, const char *title)
{
    if (!glfwInit())
    {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    std::shared_ptr<GLFWwindow> result(window, glfwDestroyWindow);
    if (!window) 
    {
        const char* err;
        glfwGetError(&err);
        LOG(err);
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    return result;
}