//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include <Engine/Renderer/Renderer.h>
#include <Engine/Renderer/GPUDevice.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/Buffer.h>
#include <Engine/Renderer/Vertex.h>
#include <Engine/Window/Window.h>

#include <array>
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
            "Assets/Shaders/Compiled/Triangle.vert.msl",
            SDL_GPU_SHADERSTAGE_VERTEX);

        const Shader fragmentShader(
            m_device->Get(),
            "Assets/Shaders/Compiled/Triangle.frag.msl",
            SDL_GPU_SHADERSTAGE_FRAGMENT);

        const SDL_GPUTextureFormat colorFormat = SDL_GetGPUSwapchainTextureFormat(
            m_device->Get(),
            m_window->GetNativeWindow());

        // Actual depth texture creation and render pass wiring land in the
        // next step; the pipeline only needs to know the format up front.
        constexpr SDL_GPUTextureFormat depthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

        m_pipeline = std::make_unique<Pipeline>(
            m_device->Get(), colorFormat, depthFormat, vertexShader, fragmentShader);

        constexpr std::array<Vertex, 3> vertices{
            Vertex{{0.0f, 0.5f}, {1.0f, 0.0f, 0.0f}},
            Vertex{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            Vertex{{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        };

        m_vertexBuffer = std::make_unique<Buffer<Vertex> >(
            m_device->Get(), SDL_GPU_BUFFERUSAGE_VERTEX, std::span(vertices));
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

        m_renderPass = SDL_BeginGPURenderPass(
            m_commandBuffer,
            &colorTarget,
            1,
            nullptr);

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

        // m_vertexBuffer/m_pipeline release themselves via their own
        // destructors (declared after m_device, so they run first).
        // m_device itself is destroyed last.
        SDL_ReleaseWindowFromGPUDevice(
            m_device->Get(),
            m_window->GetNativeWindow());
    }

    void Renderer::Render() {
        if (!m_renderPass)
            return;

        SDL_BindGPUGraphicsPipeline(m_renderPass, m_pipeline->Get());

        SDL_GPUBufferBinding binding{};
        binding.buffer = m_vertexBuffer->Get();
        binding.offset = 0;

        SDL_BindGPUVertexBuffers(m_renderPass, 0, &binding, 1);

        SDL_DrawGPUPrimitives(m_renderPass, m_vertexBuffer->Count(), 1, 0, 0);
    }
}
