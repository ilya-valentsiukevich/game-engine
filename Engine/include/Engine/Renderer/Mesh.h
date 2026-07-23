//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/Buffer.h>
#include <Engine/Renderer/Vertex.h>

#include <span>

namespace Engine {
    // Owns a vertex + index buffer pair and knows how to bind+draw itself.
    class Mesh {
    public:
        Mesh(SDL_GPUDevice *device,
             std::span<const Vertex> vertices,
             std::span<const Uint16> indices);

        Mesh(const Mesh &) = delete;
        Mesh &operator=(const Mesh &) = delete;

        void Draw(SDL_GPURenderPass *renderPass) const;

    private:
        Buffer<Vertex> m_vertexBuffer;
        Buffer<Uint16> m_indexBuffer;
    };
}
