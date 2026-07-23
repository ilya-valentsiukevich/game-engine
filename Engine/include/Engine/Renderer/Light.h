#pragma once

#include <Engine/Renderer/GlmConfig.h>

#include <glm/glm.hpp>

namespace Engine {
    // A single directional light (e.g. a sun) shared by every fragment in
    // the scene. Direction points the way the light travels — from the
    // light toward the scene, not toward the light.
    struct DirectionalLight {
        glm::vec3 Direction{0.0f, -1.0f, 0.0f};
        glm::vec3 Color{1.0f, 1.0f, 1.0f};
        float AmbientStrength = 0.15f;
        float SpecularStrength = 0.5f;
        float Shininess = 32.0f;
    };
}
