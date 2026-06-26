#pragma once

#include <future>
#include <memory>
#include <string>
#include <vector>

#include "Camera.h"
#include "Window.h"
#include "renderer/VulkanContext.h"
#include "world/World.h"

class VoxelEngine
{
public:
    VoxelEngine();
    ~VoxelEngine();

    void run();

private:
    void switchWorld(const std::string& worldName) const;
    void savePlayerState() const;
    void loadPlayerState(const std::string& worldName) const;

    std::unique_ptr<Window> m_window;
    std::unique_ptr<class Input> m_input;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<World> m_world;
    VulkanContext m_vulkanContext;

    float m_clickCooldown      = 0.0f;
    bool m_cursorEnabled       = false;
    bool m_tabPressedLastFrame = false;
    std::string m_pendingWorldLoad;

    float m_worldSizeUpdateTimer = 1.0f;
    uintmax_t m_currentWorldSize = 0;
    std::future<uintmax_t> m_worldSizeFuture;
    std::future<void> m_backgroundTask;

    std::vector<float> m_frameTimes;
    std::vector<float> m_gpuFrameTimes;
    size_t m_frameTimeIndex  = 0;
    float m_graphUpdateTimer = 0.0f;

    const uint32_t m_width  = 1600;
    const uint32_t m_height = 900;

    void initVulkan();
    void mainLoop();
    void cleanup();
};
