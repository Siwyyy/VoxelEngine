#include "Engine.h"
#include "../input/GLFWInput.h"

VoxelEngine::VoxelEngine()
{
    m_window = std::make_unique<Window>(m_width, m_height, "Voxel Engine");
    m_input = std::make_unique<GLFWInput>(m_window->getGLFWwindow());
}

VoxelEngine::~VoxelEngine() = default;

void VoxelEngine::run()
{
    initVulkan();
    mainLoop();
    cleanup();
}

void VoxelEngine::initVulkan()
{
    m_vulkanContext.init(m_window->getGLFWwindow());
}

void VoxelEngine::mainLoop()
{
    while (!m_window->shouldClose())
    {
        m_window->pollEvents();
        m_vulkanContext.drawFrame();
    }
}

void VoxelEngine::cleanup()
{
    m_vulkanContext.deviceWaitIdle();
    m_vulkanContext.cleanup();
}
