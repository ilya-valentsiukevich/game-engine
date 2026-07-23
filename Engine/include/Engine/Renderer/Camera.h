#pragma once

#include <Engine/Core/Input.h>
#include <Engine/Renderer/GlmConfig.h>

#include <glm/glm.hpp>

namespace Engine {
    // Free-flying (noclip) camera: world position + yaw/pitch Euler angles.
    // WASD moves along the camera's own forward/right axes, Space/Left Shift
    // move along world up, mouse motion rotates yaw/pitch. No roll — a fly
    // camera doesn't need it, and dropping it avoids gimbal-lock-style
    // complexity for no benefit.
    class Camera {
    public:
        void Update(float deltaTime, Input &input);

        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetProjectionMatrix(float aspectRatio) const;

        const glm::vec3 &GetPosition() const { return m_position; }

    private:
        glm::vec3 GetForward() const;

        glm::vec3 m_position{0.0f, 1.5f, 6.0f};
        glm::vec3 m_worldUp{0.0f, 1.0f, 0.0f};

        // Degrees. Yaw -90 faces -Z, matching the camera's original look
        // direction from M2's hardcoded glm::lookAt.
        float m_yaw = -90.0f;
        float m_pitch = 0.0f;

        float m_moveSpeed = 4.0f;         // world units / second
        float m_mouseSensitivity = 0.1f;  // degrees / pixel of mouse motion

        float m_fovYDegrees = 60.0f;
        float m_nearPlane = 0.1f;
        float m_farPlane = 100.0f;
    };
}
