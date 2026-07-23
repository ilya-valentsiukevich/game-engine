#pragma once

#include <Engine/Scene/SceneNode.h>

#include <memory>

namespace Engine {
    // Owns the scene tree's root node. Update() is the entry point into
    // the recursive traversal SceneNode itself implements — kept here so
    // callers don't need to know the root's parentWorld is the identity
    // matrix.
    class Scene {
    public:
        Scene();

        SceneNode &GetRoot() { return *m_root; }
        const SceneNode &GetRoot() const { return *m_root; }

        void Update();

    private:
        std::unique_ptr<SceneNode> m_root;
    };
}
