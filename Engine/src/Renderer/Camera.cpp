#include <Engine/Renderer/Camera.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace Engine {
    void Camera::Update(float deltaTime, Input &input) {
        const auto [mouseDeltaX, mouseDeltaY] = input.ConsumeMouseDelta();

        m_yaw += mouseDeltaX * m_mouseSensitivity;
        // Screen-space Y grows downward, so moving the mouse up (negative
        // yrel) should increase pitch (look up).
        m_pitch -= mouseDeltaY * m_mouseSensitivity;
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

        const glm::vec3 forward = GetForward();
        // cross(worldUp, forward), NOT cross(forward, worldUp) — matches the
        // basis glm::lookAt builds internally under GLM_FORCE_LEFT_HANDED
        // (see GlmConfig.h and docs/M3-camera-and-input.md, §1.3).
        // Swapping the operand order mirrors A/D on screen.
        const glm::vec3 right = glm::normalize(glm::cross(m_worldUp, forward));

        glm::vec3 moveDirection{0.0f};
        if (input.IsKeyDown(SDL_SCANCODE_W)) moveDirection += forward;
        if (input.IsKeyDown(SDL_SCANCODE_S)) moveDirection -= forward;
        if (input.IsKeyDown(SDL_SCANCODE_D)) moveDirection += right;
        if (input.IsKeyDown(SDL_SCANCODE_A)) moveDirection -= right;
        if (input.IsKeyDown(SDL_SCANCODE_SPACE)) moveDirection += m_worldUp;
        if (input.IsKeyDown(SDL_SCANCODE_LSHIFT)) moveDirection -= m_worldUp;

        if (glm::length(moveDirection) > 0.0f) {
            m_position += glm::normalize(moveDirection) * m_moveSpeed * deltaTime;
        }
    }

    glm::mat4 Camera::GetViewMatrix() const {
        return glm::lookAt(m_position, m_position + GetForward(), m_worldUp);
    }

    glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const {
        return glm::perspective(
            glm::radians(m_fovYDegrees), aspectRatio, m_nearPlane, m_farPlane);
    }

    glm::vec3 Camera::GetForward() const {
        const float yawRad = glm::radians(m_yaw);
        const float pitchRad = glm::radians(m_pitch);

        return glm::normalize(glm::vec3(
            std::cos(pitchRad) * std::cos(yawRad),
            std::sin(pitchRad),
            std::cos(pitchRad) * std::sin(yawRad)));
    }
}
