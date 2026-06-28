#include "Camera.h"

#include <algorithm>

#include "input/Input.h"
#include "input/InputCodes.h"

namespace voxl
{
    Camera::Camera(glm::vec3 startPosition, glm::vec3 startUp, float startYaw, float startPitch)
        : m_position(startPosition), m_worldUp(startUp), m_yaw(startYaw), m_pitch(startPitch) { updateCameraVectors(); }

    void Camera::update(const float deltaTime)
    {
        // Ruch klawiaturą
        const float velocity = m_movementSpeed * deltaTime;
        if (Input::isKeyPressed(KeyCode::W)) m_position += m_front * velocity;
        if (Input::isKeyPressed(KeyCode::S)) m_position -= m_front * velocity;
        if (Input::isKeyPressed(KeyCode::A)) m_position -= m_right * velocity;
        if (Input::isKeyPressed(KeyCode::D)) m_position += m_right * velocity;
        if (Input::isKeyPressed(KeyCode::Space)) m_position += m_worldUp * velocity;
        if (Input::isKeyPressed(KeyCode::LeftShift)) m_position -= m_worldUp * velocity;

        // Obrót myszką
        const glm::vec2 mousePos = Input::getMousePosition();
        const float xpos         = mousePos.x;
        const float ypos         = mousePos.y;

        if (m_firstMouse)
        {
            m_lastX      = xpos;
            m_lastY      = ypos;
            m_firstMouse = false;
        }

        float xoffset = xpos - m_lastX;
        float yoffset = m_lastY - ypos; // Odwrócone, ponieważ koordynaty Y idą od góry do dołu okna
        m_lastX       = xpos;
        m_lastY       = ypos;

        xoffset *= m_mouseSensitivity;
        yoffset *= m_mouseSensitivity;

        m_yaw   += xoffset;
        m_pitch += yoffset;

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
