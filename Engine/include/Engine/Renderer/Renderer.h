//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

// SDL_gpu's clip space is left-handed with depth in [0, 1] (D3D12/Metal
// convention), unlike glm's OpenGL-style defaults. Must be defined before
// the first glm include anywhere in the project.
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <Engine/Renderer/GPUResource.h>

#include <SDL3/SDL.h>
#include <memory>

namespace Engine {
    class GPUDevice;
    class Window;
    class Pipeline;
    class Mesh;

    class Renderer {
    public:
        explicit Renderer(Window &window);
        ~Renderer();

        Renderer(const Renderer &) = delete;
        Renderer &operator=(const Renderer &) = delete;

        bool BeginFrame();

        void EndFrame();

        void Update(float deltaTime);

        void Render();

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
        std::unique_ptr<Mesh> m_mesh;

        float m_rotationAngle = 0.0f;
    };
}
