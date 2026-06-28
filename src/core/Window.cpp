#include "Window.h"

#include <stdexcept>

namespace voxl
{
    Window::Window(uint32_t width, uint32_t height, std::string title)
        : m_width(width), m_height(height), m_title(std::move(title)), m_window(nullptr)
    {
        initWindow();
    }

    Window::~Window()
    {
        if (m_window) { glfwDestroyWindow(m_window); }
        glfwTerminate();
    }

    bool Window::shouldClose() const { return glfwWindowShouldClose(m_window); }


    void Window::initWindow()
    {
        if (!glfwInit()) { throw std::runtime_error("Failed to initialize GLFW!"); }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(static_cast<int>(m_width), static_cast<int>(m_height), m_title.c_str(), nullptr,
                                    nullptr);

        if (!m_window) { throw std::runtime_error("Failed to create GLFW window!"); }
    }
} // namespace voxl
