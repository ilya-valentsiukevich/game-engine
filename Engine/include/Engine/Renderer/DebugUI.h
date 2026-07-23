#pragma once

#include <Engine/Core/AppMode.h>
#include <Engine/Renderer/GPUResource.h>
#include <Engine/Renderer/IBLSettings.h>
#include <Engine/Window/Window.h>

#include <SDL3/SDL.h>
#include <entt/entt.hpp>

namespace Engine {
    // Owns the Dear ImGui context and its two backends: imgui_impl_sdl3
    // (platform — window/input/clipboard/cursor) and imgui_impl_sdlgpu3
    // (renderer — turns ImGui's draw lists into SDL_gpu buffers and draw
    // calls). One instance per application, constructed once the GPU device
    // and swapchain color format are known.
    class DebugUI {
    public:
        DebugUI(Window &window, SDL_GPUDevice *device, SDL_GPUTextureFormat colorFormat);
        ~DebugUI();

        DebugUI(const DebugUI &) = delete;
        DebugUI &operator=(const DebugUI &) = delete;

        // Forwards one polled SDL event to ImGui so it can track mouse/
        // keyboard/clipboard state, regardless of whether the engine's own
        // PollEvents switch also handles that same event.
        void ProcessEvent(const SDL_Event &event);

        // Builds this frame's ImGui windows. The FPS overlay is always
        // built; the Entities list, Inspector, and IBL window only in
        // AppMode::Debug — in AppMode::Game the mouse is captured by the
        // camera, so they couldn't be clicked anyway. Pure CPU-side widget
        // construction, no GPU calls — can run at any point before
        // FinalizeDrawData. iblSettings is edited in place by the IBL
        // window's sliders, the same pattern DrawInspector already uses for
        // component fields.
        void Draw(entt::registry &registry, AppMode mode, IBLSettings &iblSettings);

        // Finalizes this frame's draw data and uploads vertex/index buffers
        // to the GPU. Must be called while no render pass is active: this
        // does copy-pass work internally, and SDL_gpu forbids copy-pass
        // work inside a render pass.
        void FinalizeDrawData(SDL_GPUCommandBuffer *commandBuffer);

        // Issues this frame's ImGui draw calls. Must be called inside an
        // active render pass whose color target format matches the one
        // DebugUI was constructed with.
        void RenderDrawData(SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass);

    private:
        void DrawOverlay();
        void DrawEntityList(entt::registry &registry);
        void DrawInspector(entt::registry &registry);
        void DrawIBLSettings(IBLSettings &iblSettings);

        // Which entity list row was last clicked — UI-only presentation
        // state, not simulation data, so it lives here rather than as a
        // component in the registry.
        entt::entity m_selectedEntity = entt::null;
    };
}
