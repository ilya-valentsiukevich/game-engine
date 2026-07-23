#include <Engine/Scene/SceneNode.h>

namespace Engine {
    SceneNode::SceneNode(std::string name) : m_name(std::move(name)) {
    }

    SceneNode &SceneNode::AddChild(std::unique_ptr<SceneNode> child) {
        child->m_parent = this;

        // The new child starts m_localDirty (default-constructed), so it
        // will recompute itself once UpdateWorldTransform actually reaches
        // it — but only if this node (and everything above it) isn't
        // skipped as "unchanged" first, hence marking the subtree dirty
        // here too, not just on the child itself.
        if (!m_subtreeDirty) {
            m_subtreeDirty = true;
            MarkAncestorsSubtreeDirty();
        }

        m_children.push_back(std::move(child));
        return *m_children.back();
    }

    void SceneNode::UpdateWorldTransform(const glm::mat4 &parentWorld, bool parentChanged) {
        const bool selfChanged = parentChanged || m_localDirty;

        if (selfChanged) {
            m_worldMatrix = parentWorld * m_localTransform.ToMatrix();
            m_localDirty = false;
        }

        if (!selfChanged && !m_subtreeDirty)
            return; // neither this node nor anything under it changed

        m_subtreeDirty = false;

        for (const std::unique_ptr<SceneNode> &child : m_children) {
            child->UpdateWorldTransform(m_worldMatrix, selfChanged);
        }
    }

    void SceneNode::MarkLocalDirty() {
        m_localDirty = true;
        MarkAncestorsSubtreeDirty();
    }

    void SceneNode::MarkAncestorsSubtreeDirty() {
        for (SceneNode *node = m_parent; node; node = node->m_parent) {
            if (node->m_subtreeDirty)
                break; // node and everything above it are already marked

            node->m_subtreeDirty = true;
        }
    }
}
