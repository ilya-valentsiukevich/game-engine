//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include <Engine/Core/AppMode.h>
#include <Engine/Renderer/GlmConfig.h>
#include <Engine/Renderer/GPUResource.h>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <memory>

namespace Engine {
    class GPUDevice;
    class Window;
    class Pipeline;
    class Sampler;
    class DebugUI;
    class Scene;

    class Renderer {
    public:
        explicit Renderer(Window &window);
        ~Renderer();

        Renderer(const Renderer &) = delete;
        Renderer &operator=(const Renderer &) = delete;

        bool BeginFrame();

        void EndFrame();

        // Reads Scene's registry to draw the frame; not const because
        // DebugUI's inspector (drawn as part of this same pass) edits
        // component values in place — see DebugUI::DrawInspector.
        void Render(Scene &scene, AppMode mode);

        void ProcessDebugUIEvent(const SDL_Event &event);

        // Recreates every render target sized to the window (currently:
        // the depth texture) after SDL reports the window's pixel size
        // changed. Safe to call between frames only — never while a render
        // pass referencing the old target is still open.
        void OnWindowResized();

        // Non-owning access to the GPU device and default sampler backing
        // this renderer's pipeline — threaded into Scene so it can turn
        // asset paths into GPU-backed models without Scene owning any GPU
        // objects itself.
        SDL_GPUDevice *GetDevice() const;
        const Sampler &GetDefaultSampler() const;

    private:
        static constexpr SDL_GPUTextureFormat kDepthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

        void CreateDepthTexture(Uint32 width, Uint32 height);

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
        std::unique_ptr<DebugUI> m_debugUI;
    };
}
