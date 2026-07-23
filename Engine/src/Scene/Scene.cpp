#include <Engine/Scene/Scene.h>

namespace Engine {
    Scene::Scene() : m_root(std::make_unique<SceneNode>("Root")) {
    }

    void Scene::Update() {
        // false: the root has no parent to have "changed" — its own dirty
        // flag (or its subtree's) decides whether anything is recomputed.
        m_root->UpdateWorldTransform(glm::mat4(1.0f), false);
    }
}
