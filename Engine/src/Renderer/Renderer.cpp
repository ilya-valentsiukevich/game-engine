//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include <Engine/Renderer/Renderer.h>
#include <Engine/Renderer/GPUDevice.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/Model.h>
#include <Engine/Renderer/Camera.h>
#include <Engine/Renderer/Sampler.h>
#include <Engine/Window/Window.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <format>

namespace Engine {
    Renderer::Renderer(Window &window)
        : m_window(&window), m_device(std::make_unique<GPUDevice>()) {
        SDL_Log(
            "GPU Driver: %s",
            SDL_GetGPUDeviceDriver(m_device->Get())
        );

        if (!SDL_ClaimWindowForGPUDevice(
            m_device->Get(),
            m_window->GetNativeWindow())) {
            throw std::runtime_error(
                std::format("Failed to claim window: {}", SDL_GetError()));
        }

        const Shader vertexShader(
            m_device->Get(),
            "Assets/Shaders/Compiled/Mesh.vert.msl",
            SDL_GPU_SHADERSTAGE_VERTEX,
            1);

        const Shader fragmentShader(
            m_device->Get(),
            "Assets/Shaders/Compiled/Mesh.frag.msl",
            SDL_GPU_SHADERSTAGE_FRAGMENT,
            0,  // numUniformBuffers
            1); // numSamplers

        const SDL_GPUTextureFormat colorFormat = SDL_GetGPUSwapchainTextureFormat(
            m_device->Get(),
            m_window->GetNativeWindow());

        constexpr SDL_GPUTextureFormat depthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSizeInPixels(
            m_window->GetNativeWindow(), &windowWidth, &windowHeight);

        SDL_GPUTextureCreateInfo depthCreateInfo{};
        depthCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
        depthCreateInfo.format = depthFormat;
        depthCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        depthCreateInfo.width = static_cast<Uint32>(windowWidth);
        depthCreateInfo.height = static_cast<Uint32>(windowHeight);
        depthCreateInfo.layer_count_or_depth = 1;
        depthCreateInfo.num_levels = 1;
        depthCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

        m_depthTexture = GPUTextureHandle(
            m_device->Get(), SDL_CreateGPUTexture(m_device->Get(), &depthCreateInfo));

        if (!m_depthTexture) {
            throw std::runtime_error(
                std::format("Failed to create depth texture: {}", SDL_GetError()));
        }

        m_pipeline = std::make_unique<Pipeline>(
            m_device->Get(), colorFormat, depthFormat, vertexShader, fragmentShader);

        m_sampler = std::make_unique<Sampler>(
            m_device->Get(),
            SDL_GPU_FILTER_LINEAR,
            SDL_GPU_SAMPLERADDRESSMODE_REPEAT);

        m_model = std::make_unique<Model>(
            m_device->Get(), "Assets/Models/Knight/Knight.glb", *m_sampler);

        // Diorama: a "Platform" node that a handful of Knight instances are
        // parented to. Rotating just the platform's LocalTransform in
        // Update() rotates all of them together — the whole point of a
        // scene graph over a flat list (M6 §1.1).
        SceneNode &platform =
                m_scene.GetRoot().AddChild(std::make_unique<SceneNode>("Platform"));
        m_platformNode = &platform;

        constexpr int kKnightCount = 4;
        constexpr float kRadius = 2.5f;

        for (int i = 0; i < kKnightCount; ++i) {
            const float angle =
                    glm::radians(360.0f / static_cast<float>(kKnightCount) * static_cast<float>(i));

            auto knight = std::make_unique<SceneNode>(std::format("Knight{}", i));
            knight->LocalTransform.Position =
                    glm::vec3(std::cos(angle) * kRadius, 0.0f, std::sin(angle) * kRadius);
            knight->LocalTransform.Rotation =
                    glm::angleAxis(-angle, glm::vec3(0.0f, 1.0f, 0.0f));
            knight->AttachedModel = m_model.get();

            platform.AddChild(std::move(knight));
        }
    }

    bool Renderer::BeginFrame() {
        m_commandBuffer = SDL_AcquireGPUCommandBuffer(m_device->Get());

        if (!m_commandBuffer) {
            SDL_Log("Failed to acquire command buffer.");
            return false;
        }

        Uint32 width = 0;
        Uint32 height = 0;

        if (!SDL_AcquireGPUSwapchainTexture(
            m_commandBuffer,
            m_window->GetNativeWindow(),
            &m_swapchainTexture,
            &width,
            &height)) {
            SDL_Log("Failed to acquire swapchain texture.");

            SDL_CancelGPUCommandBuffer(m_commandBuffer);

            m_commandBuffer = nullptr;
            return false;
        }

        if (m_swapchainTexture == nullptr) {
            SDL_SubmitGPUCommandBuffer(m_commandBuffer);

            m_commandBuffer = nullptr;
            return false;
        }

        SDL_GPUColorTargetInfo colorTarget{};
        colorTarget.texture = m_swapchainTexture;

        colorTarget.clear_color = {
            0.0f,
            0.2f,
            1.0f,
            1.0f
        };

        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPUDepthStencilTargetInfo depthTarget{};
        depthTarget.texture = m_depthTexture.Get();
        depthTarget.clear_depth = 1.0f;
        depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

        m_renderPass = SDL_BeginGPURenderPass(
            m_commandBuffer,
            &colorTarget,
            1,
            &depthTarget);

        return true;
    }

    void Renderer::EndFrame() {
        if (!m_commandBuffer)
            return;

        if (m_renderPass) {
            SDL_EndGPURenderPass(m_renderPass);
            m_renderPass = nullptr;
        }

        SDL_SubmitGPUCommandBuffer(m_commandBuffer);

        m_commandBuffer = nullptr;
        m_swapchainTexture = nullptr;
    }

    Renderer::~Renderer() {
        if (!m_device)
            return;

        // m_depthTexture/m_pipeline/m_sampler/m_model release themselves via
        // their own destructors (declared after m_device, so they run first).
        // m_device itself is destroyed last.
        SDL_ReleaseWindowFromGPUDevice(
            m_device->Get(),
            m_window->GetNativeWindow());
    }

    void Renderer::Update(float deltaTime) {
        constexpr float kRotationSpeed = glm::radians(30.0f); // rad/sec
        m_platformRotationAngle += kRotationSpeed * deltaTime;

        if (m_platformNode) {
            m_platformNode->LocalTransform.Rotation =
                    glm::angleAxis(m_platformRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        m_scene.Update();
    }

    void Renderer::Render(const Camera &camera) {
        if (!m_renderPass)
            return;

        SDL_BindGPUGraphicsPipeline(m_renderPass, m_pipeline->Get());

        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSizeInPixels(m_window->GetNativeWindow(), &windowWidth, &windowHeight);
        const float aspectRatio =
                static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

        const glm::mat4 view = camera.GetViewMatrix();
        const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
        const glm::mat4 viewProjection = projection * view;

        DrawNode(m_scene.GetRoot(), viewProjection);
    }

    void Renderer::DrawNode(const SceneNode &node, const glm::mat4 &viewProjection) {
        if (node.AttachedModel) {
            const glm::mat4 mvp = viewProjection * node.GetWorldMatrix();
            SDL_PushGPUVertexUniformData(m_commandBuffer, 0, &mvp, sizeof(mvp));
            node.AttachedModel->Draw(m_renderPass);
        }

        for (const std::unique_ptr<SceneNode> &child : node.GetChildren()) {
            DrawNode(*child, viewProjection);
        }
    }
}
