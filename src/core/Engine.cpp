#include "Engine.h"
#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "../input/GLFWInput.h"
#include "../input/KeyCodes.h"

VoxelEngine::VoxelEngine()
{
    m_window = std::make_unique<Window>(m_width, m_height, "Voxel Engine");
    m_input = std::make_unique<GLFWInput>(m_window->getGLFWwindow());
    m_camera = std::make_unique<Camera>(glm::vec3(1.6f, 1.5f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -15.0f);

    glfwSetInputMode(m_window->getGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
    
    m_world = std::make_unique<World>(m_vulkanContext.getDevice(), m_vulkanContext.getAllocator());
}

void VoxelEngine::mainLoop()
{
    m_lastTime = std::chrono::high_resolution_clock::now();

    while (!m_window->shouldClose())
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - m_lastTime).count();
        m_lastTime = currentTime;

        m_window->pollEvents();

        if (Input::isKeyPressed(LAVA_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose(m_window->getGLFWwindow(), GLFW_TRUE);
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Debug Info");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        
        auto activeChunks = m_world->getActiveChunks();
        ImGui::Text("Render Distance: %d", 8);
        ImGui::Text("Active Chunks: %zu", activeChunks.size());
        ImGui::Text("Drawn Chunks: %u", m_vulkanContext.getDrawnChunksCount());
        
        uint32_t totalVertices = 0;
        uint32_t totalIndices = 0;
        for (const auto& chunk : activeChunks) {
            totalIndices += chunk->getIndexCount();
        }
        totalVertices = (totalIndices / 6) * 4;
        
        ImGui::Text("Total Quads (Faces): %u", totalIndices / 6);
        ImGui::Text("Total Vertices: %u", totalVertices);
        ImGui::Text("Total Indices: %u", totalIndices);
        
        ImGui::End();

        // Rysowanie celownika (crosshair) na środku ekranu
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        float crosshairSize = 8.0f;
        ImU32 crosshairColor = IM_COL32(255, 255, 255, 200); // lekko przezroczysty biały
        float thickness = 2.0f;
        
        drawList->AddLine(ImVec2(center.x - crosshairSize, center.y), ImVec2(center.x + crosshairSize, center.y), crosshairColor, thickness);
        drawList->AddLine(ImVec2(center.x, center.y - crosshairSize), ImVec2(center.x, center.y + crosshairSize), crosshairColor, thickness);

        ImGui::Render();

        m_camera->update(deltaTime);
        m_world->update(m_camera->getPosition());
        m_vulkanContext.drawFrame(m_camera->getViewMatrix(), m_world->getActiveChunks());
    }
}

void VoxelEngine::cleanup()
{
    m_vulkanContext.deviceWaitIdle();
    m_world.reset();
    m_vulkanContext.cleanup();
}
