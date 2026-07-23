#pragma once

#include <Engine/Renderer/GlmConfig.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Engine {
    // Local position/rotation/scale of a single scene node — everything
    // needed to build one 4x4 local transform matrix. Rotation is a
    // quaternion rather than Euler angles: a node's rotation gets composed
    // with its parent's every frame, and Euler angles don't compose
    // correctly under repeated composition and are prone to gimbal lock,
    // unlike Camera's yaw/pitch, which is never combined with anything.
    struct Transform {
        glm::vec3 Position{0.0f, 0.0f, 0.0f};
        glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f}; // identity (w, x, y, z)
        glm::vec3 Scale{1.0f, 1.0f, 1.0f};

        glm::mat4 ToMatrix() const;
    };
}
