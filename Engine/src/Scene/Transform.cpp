#include <Engine/Scene/Transform.h>

#include <glm/gtc/matrix_transform.hpp>

namespace Engine {
    glm::mat4 Transform::ToMatrix() const {
        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), Position);
        const glm::mat4 rotation = glm::mat4_cast(Rotation);
        const glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scale);

        // T * R * S — see M6 §1.2 for why this order, not any other.
        return translation * rotation * scale;
    }
}
