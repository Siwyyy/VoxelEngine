#pragma once

#include "../renderer/VulkanContext.h"

class VoxelEngine
{
public:
    void run();

private:
    GLFWwindow* m_window = nullptr;
    VulkanContext m_vulkanContext;

    const uint32_t m_width = 800;
    const uint32_t m_height = 600;

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
};