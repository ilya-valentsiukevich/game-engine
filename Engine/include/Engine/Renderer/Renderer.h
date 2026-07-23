//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include <Engine/Assets/AssetManager.h>
#include <Engine/Core/Input.h>
#include <Engine/Renderer/GlmConfig.h>
#include <Engine/Renderer/GPUResource.h>

#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>

namespace Engine {
    class GPUDevice;
    class Window;
    class Pipeline;
    class Sampler;

    class Renderer {
    public:
        explicit Renderer(Window &window);
        ~Renderer();

        Renderer(const Renderer &) = delete;
        Renderer &operator=(const Renderer &) = delete;

        bool BeginFrame();

        void EndFrame();

        void Update(float deltaTime, Input &input);

        void Render();

        // Re-stats every cached asset (currently: Textures) and swaps in
        // any that changed on disk since the last load or reload.
        void ReloadChangedAssets();

    private:
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

        // Owns every entity in the scene: the diorama characters
        // (Transform + MeshRenderer + Spin), the camera (Camera) and the
        // light (DirectionalLight). Declared after m_device/m_assets so it
        // is destroyed first, while the GPU device backing any AssetHandle
        // still held by a MeshRenderer component is still alive.
        entt::registry m_registry;

        // Convenience handles for the one camera and one light entity this
        // milestone creates, so Update()/Render() don't have to search the
        // registry for them every frame.
        entt::entity m_cameraEntity = entt::null;
        entt::entity m_lightEntity = entt::null;

        float m_lightAngle = 0.0f;
    };
}
