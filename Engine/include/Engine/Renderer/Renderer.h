//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include <Engine/Renderer/GlmConfig.h>
#include <Engine/Renderer/GPUResource.h>
#include <Engine/Scene/Scene.h>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <memory>

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

    private:
        // Recursively draws node and every descendant that has an
        // AttachedModel, pushing mvp = viewProjection * node.GetWorldMatrix()
        // as the vertex uniform before each Model::Draw (see M6 §1.4/§2).
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
        std::unique_ptr<Model> m_model;

        Scene m_scene;
        // Non-owning — points at a node owned by m_scene's tree, kept
        // around only so Update() can spin it without walking the tree by
        // name every frame.
        SceneNode *m_platformNode = nullptr;

        float m_platformRotationAngle = 0.0f;
    };
}
