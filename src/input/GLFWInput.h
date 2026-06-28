#pragma once
#include "Input.h"

#include <GLFW/glfw3.h>

namespace voxl
{
    class GLFWInput final : public Input
    {
    public:
        explicit GLFWInput(GLFWwindow* window);

    private:
        [[nodiscard]] bool isKeyPressedImpl(KeyCode keyCode) const override;
        [[nodiscard]] bool isMouseButtonPressedImpl(MouseCode mouseCode) const override;
        [[nodiscard]] glm::vec2 getMousePositionImpl() const override;

        GLFWwindow* m_window;
    };
} // namespace voxl
