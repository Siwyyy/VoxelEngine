#include "Engine.h"

void VoxelEngine::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VoxelEngine::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(static_cast<int>(m_width), static_cast<int>(m_height), "Voxel Engine", nullptr, nullptr);
}

void VoxelEngine::initVulkan()
{
    m_vulkanContext.init();
}

void VoxelEngine::mainLoop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
    }
}

void VoxelEngine::cleanup()
{
    m_vulkanContext.cleanup();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}