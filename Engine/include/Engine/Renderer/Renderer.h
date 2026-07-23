//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include <SDL3/SDL.h>
#include <memory>

class GPUDevice;
class Window;
class Pipeline;
struct Vertex;

template<typename T>
class Buffer;

class Renderer {
public:
    explicit Renderer(Window& window);
    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    bool BeginFrame();

    void EndFrame();

    void Render();

private:
    Window *m_window = nullptr;

    // Declaration order matters: members below are destroyed before
    // m_device (reverse declaration order), which they need to still
    // be alive when they release their GPU resources.
    std::unique_ptr<GPUDevice> m_device;

    SDL_GPUCommandBuffer *m_commandBuffer = nullptr;
    SDL_GPUTexture *m_swapchainTexture = nullptr;
    SDL_GPURenderPass *m_renderPass = nullptr;

    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<Buffer<Vertex> > m_vertexBuffer;
};
