#include <Engine/Scene/Transform.h>

#include <glm/gtc/matrix_transform.hpp>

namespace Engine {
    glm::mat4 Transform::ToMatrix() const {
        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), Position);
        const glm::mat4 rotation = glm::mat4_cast(Rotation);
        const glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scale);

        // Translate * Rotate * Scale: a point is scaled in local space,
        // then rotated about the local origin, then moved into place —
        // any other order mixes scale into the translation axes.
        return translation * rotation * scale;
    }
}
