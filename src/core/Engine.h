#pragma once

#include "Window.h"
#include "../renderer/VulkanContext.h"
#include "../world/World.h"
#include "Camera.h"
#include <memory>
#include <vector>
#include <chrono>

class VoxelEngine
{
public:
    VoxelEngine();
    ~VoxelEngine();

    void run();

private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<class Input> m_input;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<World> m_world;
    VulkanContext m_vulkanContext;

    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTime;

    const uint32_t m_width = 1600;
    const uint32_t m_height = 900;

    void initVulkan();
    void mainLoop();
    void cleanup();
};
