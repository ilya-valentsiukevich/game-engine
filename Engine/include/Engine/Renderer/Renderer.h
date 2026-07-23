//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>

#include <SDL3/SDL.h>
#include <filesystem>
#include <memory>

class GPUDevice;
class Window;

class Renderer {
public:
    explicit Renderer(Window& window);
    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    bool BeginFrame();

    void EndFrame();

    void Render();

    bool LoadShaders();

    bool CreatePipeline();

    bool CreateVertexBuffer();

private:
    GPUShaderHandle CreateShader(
        const std::filesystem::path &path,
        SDL_GPUShaderStage stage);

private:
    Window *m_window = nullptr;

    // Declaration order matters: members below are destroyed before
    // m_device (reverse declaration order), which they need to still
    // be alive when they release their GPU resources.
    std::unique_ptr<GPUDevice> m_device;

    SDL_GPUCommandBuffer *m_commandBuffer = nullptr;
    SDL_GPUTexture *m_swapchainTexture = nullptr;
    SDL_GPURenderPass *m_renderPass = nullptr;

    GPUShaderHandle m_vertexShader;
    GPUShaderHandle m_fragmentShader;

    GPUPipelineHandle m_pipeline;
    GPUBufferHandle m_vertexBuffer;
};
