#include "GLFWInput.h"

Input* Input::s_instance = nullptr;

GLFWInput::GLFWInput(GLFWwindow* window) : m_window(window)
{
    s_instance = this;
}

bool GLFWInput::isKeyPressedImpl(int32_t keycode) const
{
    auto state = glfwGetKey(m_window, keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool GLFWInput::isMouseButtonPressedImpl(int32_t button) const
{
    auto state = glfwGetMouseButton(m_window, button);
    return state == GLFW_PRESS;
}

glm::vec2 GLFWInput::getMousePositionImpl() const
{
    double x_pos, y_pos;
    glfwGetCursorPos(m_window, &x_pos, &y_pos);
    return {static_cast<float>(x_pos), static_cast<float>(y_pos)};
}
