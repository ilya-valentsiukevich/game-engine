#include <Engine/Scene/Scene.h>

namespace Engine {
    Scene::Scene() : m_root(std::make_unique<SceneNode>("Root")) {
    }

    void Scene::Update() {
        m_root->UpdateWorldTransform(glm::mat4(1.0f));
    }
}
