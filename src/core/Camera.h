#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera(glm::vec3 startPosition, glm::vec3 startUp, float startYaw, float startPitch);

    void update(float deltaTime);

    [[nodiscard]] glm::mat4 getViewMatrix() const;
    [[nodiscard]] glm::vec3 getPosition() const { return m_position; }
    [[nodiscard]] glm::vec3 getFront() const { return m_front; }
    [[nodiscard]] float getYaw() const { return m_yaw; }
    [[nodiscard]] float getPitch() const { return m_pitch; }
    void setPosition(const glm::vec3& pos) { m_position = pos; }

    void setRotation(float yaw, float pitch)
    {
        m_yaw = yaw;
        m_pitch = pitch;
        updateCameraVectors();
    }

    void resetMouse() { m_firstMouse = true; }
    void processKeyboard(int key, float deltaTime);

private:
    void updateCameraVectors();

    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;

    float m_movementSpeed = 1.4f; // 1.4 m/s (szybkość chodu)
    float m_mouseSensitivity = 0.1f;

    bool m_firstMouse = true;
    float m_lastX = 0.0f;
    float m_lastY = 0.0f;
};
