//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include <Engine/Assets/AssetManager.h>
#include <Engine/Renderer/GlmConfig.h>
#include <Engine/Renderer/GPUResource.h>
#include <Engine/Renderer/Light.h>
#include <Engine/Scene/Scene.h>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Engine {
    class GPUDevice;
    class Window;
    class Pipeline;
    class Model;
    class Camera;
    class Sampler;

    class Renderer {
    public:
        explicit Renderer(Window &window);
        ~Renderer();

        Renderer(const Renderer &) = delete;
        Renderer &operator=(const Renderer &) = delete;

        bool BeginFrame();

        void EndFrame();

        void Update(float deltaTime);

        void Render(const Camera &camera);

        // Re-stats every cached asset (currently: Textures) and swaps in
        // any that changed on disk since the last load or reload.
        void ReloadChangedAssets();

    private:
        // Recursively draws node and every descendant that has an
        // AttachedModel, pushing the node's MVP and world (model) matrices
        // as the vertex uniform before each Model::Draw.
        void DrawNode(const SceneNode &node, const glm::mat4 &viewProjection);

        Window *m_window = nullptr;

        // Declaration order matters: members below are destroyed before
        // m_device (reverse declaration order), which they need to still
        // be alive when they release their GPU resources.
        std::unique_ptr<GPUDevice> m_device;

        GPUTextureHandle m_depthTexture;

        SDL_GPUCommandBuffer *m_commandBuffer = nullptr;
        SDL_GPUTexture *m_swapchainTexture = nullptr;
        SDL_GPURenderPass *m_renderPass = nullptr;

        std::unique_ptr<Pipeline> m_pipeline;
        std::unique_ptr<Sampler> m_sampler;

        AssetManager m_assets;

        // One entry per diorama character (see constructor) — a vector of
        // AssetHandle rather than a vector of Model keeps every Model's
        // address stable across reallocation, which SceneNode::AttachedModel
        // pointers below depend on. Backed by m_assets.Models, so placing
        // the same character twice would share one Model instead of
        // loading it twice.
        std::vector<AssetHandle<Model>> m_models;

        Scene m_scene;
        // Non-owning — points at a node owned by m_scene's tree, kept
        // around only so Update() can spin it without walking the tree by
        // name every frame.
        SceneNode *m_platformNode = nullptr;

        float m_platformRotationAngle = 0.0f;

        DirectionalLight m_light;
        float m_lightAngle = 0.0f;
    };
}
