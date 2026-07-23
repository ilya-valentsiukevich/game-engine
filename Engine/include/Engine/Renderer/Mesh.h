//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/Buffer.h>
#include <Engine/Renderer/Vertex.h>

#include <span>
#include <variant>

namespace Engine {
    // Owns a vertex + index buffer pair and knows how to bind+draw itself.
    // Index width is picked per mesh from the actual vertex count: most
    // meshes fit comfortably under 65536 vertices and get narrowed down to
    // 16-bit indices (half the bandwidth of 32-bit), but a large enough
    // mesh — the whole reason indices arrive here as Uint32 — keeps the
    // full 32-bit width instead of throwing, unlike the old Uint16-only
    // Mesh.
    class Mesh {
    public:
        Mesh(SDL_GPUDevice *device,
             std::span<const Vertex> vertices,
             std::span<const Uint32> indices);

        Mesh(const Mesh &) = delete;
        Mesh &operator=(const Mesh &) = delete;

        void Draw(SDL_GPURenderPass *renderPass) const;

    private:
        Buffer<Vertex> m_vertexBuffer;
        std::variant<Buffer<Uint16>, Buffer<Uint32>> m_indexBuffer;
    };
}
