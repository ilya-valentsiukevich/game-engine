//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include <Engine/Core/AppMode.h>
#include <Engine/Renderer/GlmConfig.h>
#include <Engine/Renderer/GPUResource.h>
#include <Engine/Renderer/IBLSettings.h>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <memory>

namespace Engine {
    class GPUDevice;
    class Window;
    class Pipeline;
    class Sampler;
    class ShadowMap;
    class EnvironmentMap;
    class DebugUI;
    class Scene;
    struct DirectionalLight;

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
        static constexpr Uint32 kShadowMapSize = 2048;

        void CreateDepthTexture(Uint32 width, Uint32 height);

        // Render() split into named phases: ShadowPass renders the scene's
        // depth from the light's point of view first, then MainPass draws
        // the visible frame reading that shadow map, then UIPass composites
        // the debug UI on top.

        // Draws every (Transform, MeshRenderer) entity's geometry into the
        // shadow map from the light's point of view.
        void ShadowPass(Scene &scene, const glm::mat4 &lightSpaceMatrix);

        // Opens the main color + depth render pass, draws every (Transform,
        // MeshRenderer) entity into it reading the shadow map ShadowPass
        // just filled, and ends that render pass.
        void MainPass(Scene &scene, const glm::mat4 &lightSpaceMatrix);

        // Builds this frame's DebugUI content and composites it over the
        // swapchain in its own render pass (LOAD, not CLEAR, so it draws on
        // top of whatever MainPass already put there).
        void UIPass(Scene &scene, AppMode mode);

        // Draws the environment cubemap as the scene's background, after
        // MainPass's opaque geometry — the vertex shader pins every skybox
        // fragment to the far plane, and the pipeline's depth compare op is
        // LESS_OR_EQUAL with writes disabled, so this only ever paints
        // pixels nothing else already covered this frame.
        void SkyboxPass(const glm::mat4 &view, const glm::mat4 &projection);

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
        std::unique_ptr<ShadowMap> m_shadowMap;
        std::unique_ptr<Pipeline> m_shadowPipeline;
        std::unique_ptr<EnvironmentMap> m_environmentMap;
        std::unique_ptr<Pipeline> m_skyboxPipeline;
        IBLSettings m_iblSettings;
        std::unique_ptr<DebugUI> m_debugUI;
    };
}
