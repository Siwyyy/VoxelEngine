#include "Camera.h"

#include <algorithm>

#include "input/Input.h"

namespace voxl
{
    Camera::Camera(glm::vec3 startPosition, glm::vec3 startUp, float startYaw, float startPitch, const ActionManager& actionManager)
        : m_actionManager(actionManager), m_position(startPosition), m_worldUp(startUp), m_yaw(startYaw), m_pitch(startPitch) { updateCameraVectors(); }

    void Camera::update(const float deltaTime)
    {
        // Ruch klawiaturą
        const float velocity = m_movementSpeed * deltaTime;
        if (m_actionManager.isActionPressed(InputAction::MoveForward)) m_position += m_front * velocity;
        if (m_actionManager.isActionPressed(InputAction::MoveBackward)) m_position -= m_front * velocity;
        if (m_actionManager.isActionPressed(InputAction::MoveLeft)) m_position -= m_right * velocity;
        if (m_actionManager.isActionPressed(InputAction::MoveRight)) m_position += m_right * velocity;
        if (m_actionManager.isActionPressed(InputAction::MoveUp)) m_position += m_worldUp * velocity;
        if (m_actionManager.isActionPressed(InputAction::MoveDown)) m_position -= m_worldUp * velocity;

        // Obrót myszką
        const glm::vec2 mousePos = Input::getMousePosition();
        const float xPos         = mousePos.x;
        const float yPos         = mousePos.y;

        if (m_firstMouse)
        {
            m_lastX      = xPos;
            m_lastY      = yPos;
            m_firstMouse = false;
        }

        float xOffset = xPos - m_lastX;
        float yOffset = m_lastY - yPos; // Odwrócone, ponieważ koordynaty Y idą od góry do dołu okna
        m_lastX       = xPos;
        m_lastY       = yPos;

        xOffset *= m_mouseSensitivity;
        yOffset *= m_mouseSensitivity;

        m_yaw   += xOffset;
        m_pitch += yOffset;

        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

        updateCameraVectors();
    }

    glm::mat4 Camera::getViewMatrix() const { return glm::lookAt(m_position, m_position + m_front, m_up); }

    void Camera::updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = sin(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        m_front = glm::normalize(front);
        m_right = glm::normalize(glm::cross(m_front, m_worldUp));
        m_up    = glm::normalize(glm::cross(m_right, m_front));
    }
} // namespace voxl
