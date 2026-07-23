#pragma once

#include <Engine/Scene/Transform.h>
#include <Engine/Renderer/GlmConfig.h>

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Engine {
    class Model;

    // One node of the scene tree: a local Transform, an optional attached
    // Model to draw, and any number of child nodes it owns. AttachedModel
    // is non-owning — Models are loaded/owned elsewhere (currently
    // Renderer, see M6 Part 2 step 4) and can be attached to more than one
    // node at once (see the diorama built in Renderer's constructor),
    // exactly the same ownership split Material already uses for Sampler
    // (M4 §1.3).
    class SceneNode {
    public:
        explicit SceneNode(std::string name = {});

        // Returns a reference to the newly added child — stays valid even
        // after later AddChild calls reallocate the parent's internal
        // vector, since the vector holds unique_ptr<SceneNode>: only the
        // pointer values move, never the SceneNode objects themselves.
        SceneNode &AddChild(std::unique_ptr<SceneNode> child);

        // Recomputes this node's world matrix from parentWorld * LocalTransform,
        // then recurses into every child with the result — one pre-order
        // pass over the subtree rooted here (see M6 §1.4). Called once per
        // fixed Update tick from Scene::Update(), not per render frame.
        void UpdateWorldTransform(const glm::mat4 &parentWorld);

        const glm::mat4 &GetWorldMatrix() const { return m_worldMatrix; }
        const std::string &GetName() const { return m_name; }

        std::vector<std::unique_ptr<SceneNode>> &GetChildren() { return m_children; }

        const std::vector<std::unique_ptr<SceneNode>> &GetChildren() const {
            return m_children;
        }

        Transform LocalTransform;
        const Model *AttachedModel = nullptr;

    private:
        std::string m_name;
        glm::mat4 m_worldMatrix{1.0f};
        std::vector<std::unique_ptr<SceneNode>> m_children;
    };
}
