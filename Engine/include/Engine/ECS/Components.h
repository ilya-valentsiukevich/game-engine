#pragma once

#include <Engine/Assets/AssetCache.h>
#include <Engine/Renderer/GlmConfig.h>

#include <glm/glm.hpp>

#include <string>

namespace Engine {
    class Model;

    // Human-readable label for an entity, shown in DebugUI's entity list.
    // Entities without a Name simply don't appear there — this is a
    // presentation detail, not a requirement for an entity to exist or be
    // drawn.
    struct Name {
        std::string Value;
    };

    // Marks an entity as something RenderSystem should draw, paired with a
    // Transform component on the same entity. Holding the handle here (on
    // top of AssetCache's own strong reference) gives RenderSystem direct
    // access without a second cache lookup per entity per frame.
    struct MeshRenderer {
        AssetHandle<Model> Model;
    };

    // Marks an entity RotateSystem should advance every tick: both
    // Transform::Position and Transform::Rotation get multiplied by the
    // same incremental rotation around Axis. Rotating Position too — not
    // just Rotation — is what makes the entity orbit the world origin
    // instead of merely spinning in place.
    struct Spin {
        glm::vec3 Axis{0.0f, 1.0f, 0.0f};
        float AngularSpeed = 0.0f; // radians/second
    };
}
