//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include <SDL3/SDL.h>
#include <filesystem>

class Window;

class Renderer {
public:
    bool Initialize(Window &window);

    bool BeginFrame();

    void EndFrame();

    void Shutdown();

    void Render();

    bool LoadShaders();

    bool CreatePipeline();

    bool CreateVertexBuffer();

private:
    SDL_GPUShader *CreateShader(
        const std::filesystem::path &path,
        SDL_GPUShaderStage stage);

private:
    Window *m_window = nullptr;
    SDL_GPUDevice *m_device = nullptr;

    SDL_GPUCommandBuffer *m_commandBuffer = nullptr;
    SDL_GPUTexture *m_swapchainTexture = nullptr;
    SDL_GPURenderPass *m_renderPass = nullptr;

    SDL_GPUShader *m_vertexShader = nullptr;
    SDL_GPUShader *m_fragmentShader = nullptr;

    SDL_GPUGraphicsPipeline *m_pipeline = nullptr;
    SDL_GPUBuffer *m_vertexBuffer = nullptr;
};
