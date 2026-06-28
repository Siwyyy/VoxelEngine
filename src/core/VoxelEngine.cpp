#include "VoxelEngine.h"

#include <filesystem>
#include <fstream>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include "Time.h"
#include "input/GLFWInput.h"


namespace voxl
{
    VoxelEngine::VoxelEngine()
    {
        m_window = std::make_unique<Window>(m_width, m_height, "Voxel Engine");
        m_input  = std::make_unique<GLFWInput>(m_window->getGLFWwindow());
        m_camera = std::make_unique<Camera>(glm::vec3(1.6f, 1.5f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -15.0f);

        glfwSetInputMode(m_window->getGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        m_frameTimes.resize(300, 0.0f);
        m_gpuFrameTimes.resize(300, 0.0f);

        loadPlayerState("world1");
    }

    VoxelEngine::~VoxelEngine() = default;

    void VoxelEngine::run()
    {
        initVulkan();
        mainLoop();
        cleanup();
    }

    void VoxelEngine::switchWorld(const std::string& worldName) const
    {
        savePlayerState();
        m_world->changeWorld("saves/" + worldName + "/");
        loadPlayerState(worldName);
    }

    void VoxelEngine::savePlayerState() const
    {
        if (!m_world || !m_camera) return;
        const std::string currentPath = m_world->getWorldPath();
        std::ofstream out(currentPath + "player.dat", std::ios::binary);
        if (out)
        {
            const glm::vec3 cPos = m_camera->getPosition();
            const float cYaw     = m_camera->getYaw();
            const float cPitch   = m_camera->getPitch();
            out.write(reinterpret_cast<const char*>(&cPos), sizeof(cPos));
            out.write(reinterpret_cast<const char*>(&cYaw), sizeof(cYaw));
            out.write(reinterpret_cast<const char*>(&cPitch), sizeof(cPitch));
        }
    }

    void VoxelEngine::loadPlayerState(const std::string& worldName) const
    {
        std::ifstream in("saves/" + worldName + "/player.dat", std::ios::binary);
        if (in)
        {
            glm::vec3 cPos;
            float cYaw, cPitch;
            in.read(reinterpret_cast<char*>(&cPos), sizeof(cPos));
            in.read(reinterpret_cast<char*>(&cYaw), sizeof(cYaw));
            in.read(reinterpret_cast<char*>(&cPitch), sizeof(cPitch));
            m_camera->setPosition(cPos);
            m_camera->setRotation(cYaw, cPitch);
        }
        else
        {
            m_camera->setPosition(glm::vec3(1.6f, 1.5f, 5.0f));
            m_camera->setRotation(-90.0f, -15.0f);
        }
    }

    void VoxelEngine::initVulkan()
    {
        m_vulkanContext.init(m_window->getGLFWwindow());

        m_world = std::make_unique<World>(&m_vulkanContext);
    }

    void VoxelEngine::mainLoop()
    {
        Time::init();

        bool isShuttingDown = false;
        while (true)
        {
            if (glfwWindowShouldClose(m_window->getGLFWwindow()))
            {
                isShuttingDown = true;
                glfwSetWindowShouldClose(m_window->getGLFWwindow(), GLFW_FALSE);
            }

            if (isShuttingDown)
            {
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
                ImGui::Begin("Saving Screen", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

                const auto loadingText = "Saving World, please wait...";
                const ImVec2 textSize  = ImGui::CalcTextSize(loadingText);
                const auto pos         = ImVec2((ImGui::GetIO().DisplaySize.x - textSize.x) * 0.5f,
                                                (ImGui::GetIO().DisplaySize.y - textSize.y) * 0.5f);
                ImGui::SetCursorPos(pos);
                ImGui::Text("%s", loadingText);
                ImGui::End();

                ImGui::Render();

                m_vulkanContext.drawFrame(m_camera->getViewMatrix(), {});

                break;
            }

            Time::tick();
            const float deltaTime = Time::getDeltaTime();

            m_graphUpdateTimer += deltaTime;
            if (m_graphUpdateTimer >= 1.0f / 60.0f)
            {
                m_graphUpdateTimer = 0.0f;
                if (!m_frameTimes.empty())
                {
                    m_frameTimes[m_frameTimeIndex]    = deltaTime * 1000.0f; // store as ms
                    m_gpuFrameTimes[m_frameTimeIndex] = m_vulkanContext.getGpuFrameTime();
                    m_frameTimeIndex                  = (m_frameTimeIndex + 1) % m_frameTimes.size();
                }
            }

            m_window->pollEvents();

            if (!m_pendingWorldLoad.empty())
            {
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
                ImGui::Begin("Loading Screen", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

                const auto loadingText = "Loading World...";
                const ImVec2 textSize  = ImGui::CalcTextSize(loadingText);
                auto pos               = ImVec2((ImGui::GetIO().DisplaySize.x - textSize.x) * 0.5f,
                                                (ImGui::GetIO().DisplaySize.y - textSize.y) * 0.5f);
                ImGui::SetCursorPos(pos);
                ImGui::Text("%s", loadingText);
                ImGui::End();

                ImGui::Render();

                std::vector<Chunk*> emptyChunks;
                m_vulkanContext.drawFrame(m_camera->getViewMatrix(), emptyChunks);

                switchWorld(m_pendingWorldLoad);
                m_pendingWorldLoad = "";
                Time::init();
                continue;
            }

            if (Input::isKeyPressed(KeyCode::Escape)) { glfwSetWindowShouldClose(m_window->getGLFWwindow(), GLFW_TRUE); }

            const bool tabPressed = Input::isKeyPressed(KeyCode::Tab);
            if (tabPressed && !m_tabPressedLastFrame)
            {
                m_cursorEnabled = !m_cursorEnabled;
                if (m_cursorEnabled) { glfwSetInputMode(m_window->getGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL); }
                else
                {
                    glfwSetInputMode(m_window->getGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    m_camera->resetMouse();
                }
            }
            m_tabPressedLastFrame = tabPressed;

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(350, 250), ImGuiCond_FirstUseEver);
            ImGui::Begin("Debug Info");

            m_worldSizeUpdateTimer += deltaTime;
            if (m_worldSizeUpdateTimer >= 1.0f && !m_worldSizeFuture.valid())
            {
                m_worldSizeUpdateTimer = 0.0f;
                std::string pathToScan = m_world->getWorldPath();
                m_worldSizeFuture      = std::async(std::launch::async, [pathToScan]()
                {
                    uintmax_t size = 0;
                    if (std::filesystem::exists(pathToScan))
                    {
                        std::error_code ec;
                        for (const auto& entry: std::filesystem::recursive_directory_iterator(
                                 pathToScan,
                                 std::filesystem::directory_options::skip_permission_denied,
                                 ec))
                        {
                            if (entry.is_regular_file(ec)) { size += entry.file_size(ec); }
                        }
                    }
                    return size;
                });
            }

            if (m_worldSizeFuture.valid() && m_worldSizeFuture.wait_for(std::chrono::seconds(0)) ==
                std::future_status::ready) { m_currentWorldSize = m_worldSizeFuture.get(); }

            std::string pathStr         = m_world->getWorldPath();
            std::string activeWorldName = pathStr;
            if (pathStr.length() > 6) { activeWorldName = pathStr.substr(6, pathStr.length() - 7); }
            ImGui::Text("World: %s (%.2f MB)", activeWorldName.c_str(),
                        static_cast<float>(m_currentWorldSize) / (1024.0f * 1024.0f));
            ImGui::Separator();

            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

            int currentRd = m_world->getRenderDistance();
            if (ImGui::SliderInt("Render Distance", &currentRd, 2, 32)) { m_world->setRenderDistance(currentRd); }

            const auto& activeChunks = m_world->getActiveChunks();
            ImGui::Text("Active Chunks: %zu", activeChunks.size());
            ImGui::Text("Drawn Chunks: %u", m_vulkanContext.getDrawnChunksCount());

            uint32_t totalIndices = 0;
            for (const auto& chunk: activeChunks) { totalIndices += chunk->getIndexCount(); }
            const uint32_t totalVertices = (totalIndices / 6) * 4;

            ImGui::Text("Total Quads (Faces): %u", totalIndices / 6);
            ImGui::Text("Total Vertices: %u", totalVertices);
            ImGui::Text("Total Indices: %u", totalIndices);

            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(10, 270), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_FirstUseEver);
            ImGui::Begin("World Selection");
            ImGui::Text("Available Worlds:");
            if (std::filesystem::exists("saves"))
            {
                for (const auto& entry: std::filesystem::directory_iterator("saves"))
                {
                    if (entry.is_directory())
                    {
                        std::string worldName = entry.path().filename().string();
                        if (ImGui::Button(worldName.c_str())) { m_pendingWorldLoad = worldName; }
                    }
                }
            }
            ImGui::Separator();

            static std::array<char, 128> worldNameBuffer = {"new_world"};
            ImGui::InputText("New World Name", worldNameBuffer.data(), worldNameBuffer.size());
            if (ImGui::Button("Create / Load World")) { m_pendingWorldLoad = std::string(worldNameBuffer.data()); }
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(10, 480), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(350, 240), ImGuiCond_FirstUseEver);
            ImGui::Begin("Performance Profiler");

            float avgCpu = 0.0f;
            float avgGpu = 0.0f;
            for (const float ft: m_frameTimes) avgCpu += ft;
            for (const float ft: m_gpuFrameTimes) avgGpu += ft;
            avgCpu /= static_cast<float>(m_frameTimes.size());
            avgGpu /= static_cast<float>(m_gpuFrameTimes.size());

            ImGui::Text("CPU Frame Time: %.2f ms", avgCpu);
            ImGui::PlotLines("##CPUGraph", m_frameTimes.data(), static_cast<int>(m_frameTimes.size()),
                             static_cast<int>(m_frameTimeIndex),
                             "CPU Frame Time (ms)", 0.0f, 30.0f, ImVec2(ImGui::GetContentRegionAvail().x, 60));

            ImGui::Text("GPU Frame Time: %.2f ms", avgGpu);
            ImGui::PlotLines("##GPUGraph", m_gpuFrameTimes.data(), static_cast<int>(m_gpuFrameTimes.size()),
                             static_cast<int>(m_frameTimeIndex),
                             "GPU Frame Time (ms)", 0.0f, 30.0f, ImVec2(ImGui::GetContentRegionAvail().x, 60));

            ImGui::End();

            // Rysowanie celownika (crosshair) na środku ekranu
            ImDrawList* drawList           = ImGui::GetBackgroundDrawList();
            const auto center              = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
            constexpr float crosshairSize  = 8.0f;
            constexpr ImU32 crosshairColor = IM_COL32(255, 255, 255, 200);
            constexpr float thickness      = 2.0f;

            drawList->AddLine(ImVec2(center.x - crosshairSize, center.y), ImVec2(center.x + crosshairSize, center.y),
                              crosshairColor, thickness);
            drawList->AddLine(ImVec2(center.x, center.y - crosshairSize), ImVec2(center.x, center.y + crosshairSize),
                              crosshairColor, thickness);

            ImGui::Render();

            if (!m_cursorEnabled)
            {
                m_camera->update(deltaTime);

                if (m_clickCooldown > 0.0f) { m_clickCooldown -= deltaTime; }
                else
                {
                    const bool leftClick  = Input::isMouseButtonPressed(MouseCode::Left);
                    const bool rightClick = Input::isMouseButtonPressed(MouseCode::Right);

                    if (leftClick || rightClick)
                    {
                        m_world->processPlayerInteraction(m_camera->getPosition(), m_camera->getFront(), leftClick, rightClick);
                        m_clickCooldown = 0.2f;
                    }
                }
            }
            m_world->update(m_camera->getPosition(), m_camera->getFront(), deltaTime);
            m_vulkanContext.drawFrame(m_camera->getViewMatrix(), m_world->getActiveChunks());
        }
    }

    void VoxelEngine::cleanup()
    {
        savePlayerState();
        m_vulkanContext.deviceWaitIdle();
        m_world.reset();
        m_vulkanContext.cleanup();
    }
} // namespace voxl
