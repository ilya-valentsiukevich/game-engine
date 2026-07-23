#pragma once

#include <Engine/Renderer/GlmConfig.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Engine {
    // Local position/rotation/scale of a single scene node — everything
    // needed to build one 4x4 local transform matrix (see M6 §1.2).
    // Rotation is a quaternion, not Euler angles like Camera's yaw/pitch
    // (M3): unlike the camera, a node's rotation gets composed with its
    // parent's every frame (see M6 §1.3) — Euler angles don't compose
    // correctly under that operation and are prone to gimbal lock.
    struct Transform {
        glm::vec3 Position{0.0f, 0.0f, 0.0f};
        glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f}; // identity (w, x, y, z)
        glm::vec3 Scale{1.0f, 1.0f, 1.0f};

        glm::mat4 ToMatrix() const;
    };
}
