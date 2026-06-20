#pragma once

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
public:
    Window(uint32_t width, uint32_t height, const std::string& title);
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    [[nodiscard]] bool shouldClose() const;
    void pollEvents();

    [[nodiscard]] GLFWwindow* getGLFWwindow() const { return m_window; }

private:
    void initWindow();

    uint32_t m_width;
    uint32_t m_height;
    std::string m_title;
    GLFWwindow* m_window;
};
