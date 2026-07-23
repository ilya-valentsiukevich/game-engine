#pragma once

#include <Engine/Renderer/GlmConfig.h>

#include <glm/glm.hpp>

namespace Engine {
    // Maximum number of point/spot lights MainPass collects into a single
    // frame's uniform buffer. Must match the array sizes declared in
    // Mesh.frag.msl exactly — there is no shared header generating both
    // sides yet.
    inline constexpr int kMaxPointLights = 4;
    inline constexpr int kMaxSpotLights = 2;

    // A single directional light (e.g. a sun) shared by every fragment in
    // the scene. Direction points the way the light travels — from the
    // light toward the scene, not toward the light.
    struct DirectionalLight {
        glm::vec3 Direction{0.0f, -1.0f, 0.0f};
        glm::vec3 Color{1.0f, 1.0f, 1.0f};
        float AmbientStrength = 0.15f;
    };

    // A light radiating equally in every direction from Transform::Position
    // on the same entity, fading out with distance. Constant/Linear/
    // Quadratic follow the classic attenuation model:
    // attenuation = 1 / (Constant + Linear * d + Quadratic * d^2).
    struct PointLight {
        glm::vec3 Color{1.0f, 1.0f, 1.0f};
        float Constant = 1.0f;
        float Linear = 0.09f;
        float Quadratic = 0.032f;
    };

    // A point light additionally restricted to a cone around Direction,
    // with a smooth falloff between InnerConeAngleDegrees (full brightness)
    // and OuterConeAngleDegrees (no light). Direction is explicit rather
    // than derived from the entity's Transform::Rotation, for the same
    // reason DirectionalLight::Direction is explicit — see
    // DebugUI::DrawInspector, which deliberately doesn't expose the raw
    // Transform quaternion for editing.
    struct SpotLight {
        glm::vec3 Direction{0.0f, -1.0f, 0.0f};
        glm::vec3 Color{1.0f, 1.0f, 1.0f};
        float Constant = 1.0f;
        float Linear = 0.09f;
        float Quadratic = 0.032f;
        float InnerConeAngleDegrees = 12.5f;
        float OuterConeAngleDegrees = 17.5f;
    };
}
