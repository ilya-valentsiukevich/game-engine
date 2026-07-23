//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Mesh.h>

namespace Engine {
    Mesh::Mesh(SDL_GPUDevice *device,
               std::span<const Vertex> vertices,
               std::span<const Uint16> indices)
        : m_vertexBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, vertices),
          m_indexBuffer(device, SDL_GPU_BUFFERUSAGE_INDEX, indices) {
    }

    void Mesh::Draw(SDL_GPURenderPass *renderPass) const {
        SDL_GPUBufferBinding vertexBinding{};
        vertexBinding.buffer = m_vertexBuffer.Get();
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

        SDL_GPUBufferBinding indexBinding{};
        indexBinding.buffer = m_indexBuffer.Get();
        indexBinding.offset = 0;
        SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        SDL_DrawGPUIndexedPrimitives(
            renderPass, m_indexBuffer.Count(), 1, 0, 0, 0);
    }
}
