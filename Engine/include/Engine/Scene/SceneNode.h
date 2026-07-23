#pragma once

#include <Engine/ECS/Transform.h>
#include <Engine/Renderer/GlmConfig.h>

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Engine {
    class Model;

    // One node of the scene tree: a local Transform, an optional attached
    // Model to draw, and any number of child nodes it owns. AttachedModel
    // is non-owning — Models are loaded/owned elsewhere, and the same
    // Model can be attached to more than one node at once, the same
    // ownership split Material uses for Sampler.
    //
    // World matrices are cached and only recomputed when something that
    // affects them actually changed: a node recomputes when either its own
    // local transform changed since the last UpdateWorldTransform, or its
    // parent's world matrix changed. A node whose own transform is
    // unchanged still has to be *visited* if any descendant is dirty
    // (m_subtreeDirty) — otherwise that descendant's change would never
    // reach UpdateWorldTransform at all. Both flags are set by
    // GetLocalTransform()/AddChild(), which propagate up the parent chain —
    // this is why SceneNode keeps a non-owning back-pointer to its parent.
    class SceneNode {
    public:
        explicit SceneNode(std::string name = {});

        // Returns a reference to the newly added child — stays valid even
        // after later AddChild calls reallocate the parent's internal
        // vector, since the vector holds unique_ptr<SceneNode>: only the
        // pointer values move, never the SceneNode objects themselves.
        SceneNode &AddChild(std::unique_ptr<SceneNode> child);

        // Recomputes this node's world matrix from parentWorld * local
        // transform if needed (see class comment), then recurses into
        // every child — skipping the whole subtree when neither this node
        // nor anything under it changed since the last call. parentChanged
        // is true when the caller just recomputed parentWorld itself.
        // Called once per fixed Update tick from Scene::Update(), not per
        // render frame.
        void UpdateWorldTransform(const glm::mat4 &parentWorld, bool parentChanged);

        const glm::mat4 &GetWorldMatrix() const { return m_worldMatrix; }
        const std::string &GetName() const { return m_name; }

        std::vector<std::unique_ptr<SceneNode>> &GetChildren() { return m_children; }

        const std::vector<std::unique_ptr<SceneNode>> &GetChildren() const {
            return m_children;
        }

        // Read-only: never marks anything dirty.
        const Transform &GetLocalTransform() const { return m_localTransform; }

        // Mutable: always marks this node (and every ancestor, transitively)
        // dirty, whether or not the caller actually changes the returned
        // reference. Slightly conservative — a read-only call still pays
        // for a dirty walk up to the root — but a caller-supplied
        // MarkDirty() the caller has to remember to call after mutating is
        // one missed call away from a node that silently stops updating.
        Transform &GetLocalTransform() {
            MarkLocalDirty();
            return m_localTransform;
        }

        const Model *AttachedModel = nullptr;

    private:
        void MarkLocalDirty();

        void MarkAncestorsSubtreeDirty();

        std::string m_name;
        Transform m_localTransform;
        glm::mat4 m_worldMatrix{1.0f};

        SceneNode *m_parent = nullptr;
        std::vector<std::unique_ptr<SceneNode>> m_children;

        // Freshly constructed nodes start dirty so the very first
        // UpdateWorldTransform() call always computes a real world matrix
        // instead of skipping based on default-identity state.
        bool m_localDirty = true;
        bool m_subtreeDirty = false;
    };
}
