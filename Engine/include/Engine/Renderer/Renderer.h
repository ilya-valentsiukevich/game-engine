//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include <Engine/Renderer/GlmConfig.h>
#include <Engine/Renderer/GPUResource.h>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <array>
#include <memory>

namespace Engine {
    class GPUDevice;
    class Window;
    class Pipeline;
    class Mesh;
    class Camera;

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

        std::array<glm::vec3, 5> m_cubePositions{
            glm::vec3{0.0f, 0.0f, 0.0f},
            glm::vec3{3.0f, 0.0f, 0.0f},
            glm::vec3{-3.0f, 0.0f, 0.0f},
            glm::vec3{0.0f, 0.0f, -4.0f},
            glm::vec3{0.0f, 0.0f, 4.0f},
        };
    };
}
