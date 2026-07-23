#include <Engine/Scene/SceneNode.h>

namespace Engine {
    SceneNode::SceneNode(std::string name) : m_name(std::move(name)) {
    }

    SceneNode &SceneNode::AddChild(std::unique_ptr<SceneNode> child) {
        m_children.push_back(std::move(child));
        return *m_children.back();
    }

    void SceneNode::UpdateWorldTransform(const glm::mat4 &parentWorld) {
        m_worldMatrix = parentWorld * LocalTransform.ToMatrix();

        for (const std::unique_ptr<SceneNode> &child : m_children) {
            child->UpdateWorldTransform(m_worldMatrix);
        }
    }
}
