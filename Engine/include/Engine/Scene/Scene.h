#pragma once

#include <Engine/Assets/AssetManager.h>
#include <Engine/Core/AppMode.h>
#include <Engine/Core/Input.h>

#include <SDL3/SDL_gpu.h>
#include <entt/entt.hpp>

#include <filesystem>

namespace Engine {
    class Model;
    class Sampler;

    // Owns the ECS registry and the asset caches backing it — the runtime
    // state of "what exists in the world", independent of how it gets
    // drawn. Renderer only reads Scene's registry each frame (RenderSystem,
    // DebugUI); building actual scene content (camera, lights, models) is
    // up to whoever constructs a Scene — see Game/main.cpp.
    class Scene {
    public:
        // device/sampler are non-owning: both are GPU resources Renderer
        // already owns, threaded through here only so LoadModel() can turn
        // a path into GPU-backed geometry without exposing GPU internals to
        // callers building scene content.
        Scene(SDL_GPUDevice *device, const Sampler &sampler);

        Scene(const Scene &) = delete;
        Scene &operator=(const Scene &) = delete;

        // Camera only reacts to input in AppMode::Game — see AppMode.h.
        // Requires exactly one entity tagged ActiveCamera and one
        // DirectionalLight entity to already exist in the registry.
        void Update(float deltaTime, Input &input, AppMode mode);

        AssetHandle<Model> LoadModel(const std::filesystem::path &path);

        // Re-stats every cached asset (currently: Textures) and swaps in
        // any that changed on disk since the last load or reload.
        void ReloadChangedAssets();

        entt::registry &Registry() { return m_registry; }
        const entt::registry &Registry() const { return m_registry; }

    private:
        SDL_GPUDevice *m_device;
        const Sampler *m_sampler;

        AssetManager m_assets;
        entt::registry m_registry;

        // Drives the directional light's slow arc across the sky — see
        // Scene::Update.
        float m_lightAngle = 0.0f;
    };
}
