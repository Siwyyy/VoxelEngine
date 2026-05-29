#pragma once

#include "Window.h"
#include "../renderer/VulkanContext.h"
#include <memory>

class VoxelEngine
{
public:
    VoxelEngine();
    ~VoxelEngine();

    void run();

private:
    std::unique_ptr<Window> m_window;
    VulkanContext m_vulkanContext;

    const uint32_t m_width = 800;
    const uint32_t m_height = 600;

    void initVulkan();
    void mainLoop();
    void cleanup();
};