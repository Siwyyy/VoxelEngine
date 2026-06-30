#include "GLFWInput.h"

#include <utility>

namespace voxl
{
    Input* Input::s_instance = nullptr;

    GLFWInput::GLFWInput(GLFWwindow* window)
        : m_window(window)
    {
        s_instance = this;
    }

    bool GLFWInput::isKeyPressedImpl(KeyCode keyCode) const
    {
        const auto state = glfwGetKey(m_window, std::to_underlying(keyCode));
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool GLFWInput::isMouseButtonPressedImpl(MouseCode mouseCode) const
    {
        const auto state = glfwGetMouseButton(m_window, std::to_underlying(mouseCode));
        return state == GLFW_PRESS;
    }

    glm::vec2 GLFWInput::getMousePositionImpl() const
    {
        double x_pos, y_pos;
        glfwGetCursorPos(m_window, &x_pos, &y_pos);
        return {static_cast<float>(x_pos), static_cast<float>(y_pos)};
    }
} // namespace voxl
