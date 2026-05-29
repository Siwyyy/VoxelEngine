#pragma once
#include "Input.h"
#include <GLFW/glfw3.h>

class GLFWInput final : public Input
{
public:
    GLFWInput(GLFWwindow* window);
private:
    bool isKeyPressedImpl(int32_t keycode_) const override;
    bool isMouseButtonPressedImpl(int32_t button_) const override;
    glm::vec2 getMousePositionImpl() const override;

    GLFWwindow* m_window;
};
