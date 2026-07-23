//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include <Engine/Renderer/Renderer.h>
#include <Engine/Renderer/GPUDevice.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/Renderer/CubePrimitive.h>
#include <Engine/Renderer/Camera.h>
#include <Engine/Renderer/Texture.h>
#include <Engine/Renderer/Sampler.h>
#include <Engine/Window/Window.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <format>
#include <span>

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
            "Assets/Shaders/Compiled/Cube.vert.msl",
            SDL_GPU_SHADERSTAGE_VERTEX,
            1);

        const Shader fragmentShader(
            m_device->Get(),
            "Assets/Shaders/Compiled/Cube.frag.msl",
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

        m_mesh = std::make_unique<Mesh>(
            m_device->Get(), std::span(kCubeVertices), std::span(kCubeIndices));

        m_texture = std::make_unique<Texture>(
            m_device->Get(), "Assets/Textures/crate.png");

        m_sampler = std::make_unique<Sampler>(
            m_device->Get(),
            SDL_GPU_FILTER_LINEAR,
            SDL_GPU_SAMPLERADDRESSMODE_REPEAT);
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

        // m_depthTexture/m_pipeline/m_mesh/m_texture/m_sampler release
        // themselves via their own destructors (declared after m_device, so
        // they run first).
        // m_device itself is destroyed last.
        SDL_ReleaseWindowFromGPUDevice(
            m_device->Get(),
            m_window->GetNativeWindow());
    }

    void Renderer::Update(float deltaTime) {
        constexpr float kRotationSpeed = glm::radians(90.0f); // rad/sec
        m_rotationAngle += kRotationSpeed * deltaTime;
    }

    void Renderer::Render(const Camera &camera) {
        if (!m_renderPass)
            return;

        SDL_BindGPUGraphicsPipeline(m_renderPass, m_pipeline->Get());

        SDL_GPUTextureSamplerBinding textureSamplerBinding{};
        textureSamplerBinding.texture = m_texture->Get();
        textureSamplerBinding.sampler = m_sampler->Get();
        SDL_BindGPUFragmentSamplers(m_renderPass, 0, &textureSamplerBinding, 1);

        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSizeInPixels(m_window->GetNativeWindow(), &windowWidth, &windowHeight);
        const float aspectRatio =
                static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

        const glm::mat4 view = camera.GetViewMatrix();
        const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);

        for (const glm::vec3 &cubePosition : m_cubePositions) {
            const glm::mat4 model =
                    glm::translate(glm::mat4(1.0f), cubePosition) *
                    glm::rotate(glm::mat4(1.0f), m_rotationAngle, glm::vec3(0.5f, 1.0f, 0.0f));

            const glm::mat4 mvp = projection * view * model;

            SDL_PushGPUVertexUniformData(m_commandBuffer, 0, &mvp, sizeof(mvp));

            m_mesh->Draw(m_renderPass);
        }
    }
}
