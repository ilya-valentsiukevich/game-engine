//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Mesh.h>

#include <type_traits>
#include <vector>

namespace Engine {
    namespace {
        std::variant<Buffer<Uint16>, Buffer<Uint32>> CreateIndexBuffer(
            SDL_GPUDevice *device, std::span<const Vertex> vertices, std::span<const Uint32> indices) {
            // Only vertex count decides the width: it bounds every value an
            // index into this mesh could possibly hold, regardless of what
            // the source indices happened to need.
            if (vertices.size() <= 0x10000u) { // fits Uint16 (0..65535)
                std::vector<Uint16> narrowedIndices(indices.size());

                for (size_t i = 0; i < indices.size(); ++i)
                    narrowedIndices[i] = static_cast<Uint16>(indices[i]);

                return Buffer<Uint16>(device, SDL_GPU_BUFFERUSAGE_INDEX, std::span(narrowedIndices));
            }

            return Buffer<Uint32>(device, SDL_GPU_BUFFERUSAGE_INDEX, indices);
        }
    }

    Mesh::Mesh(SDL_GPUDevice *device,
               std::span<const Vertex> vertices,
               std::span<const Uint32> indices)
        : m_vertexBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, vertices),
          m_indexBuffer(CreateIndexBuffer(device, vertices, indices)) {
    }

    void Mesh::Draw(SDL_GPURenderPass *renderPass) const {
        SDL_GPUBufferBinding vertexBinding{};
        vertexBinding.buffer = m_vertexBuffer.Get();
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

        std::visit([renderPass](const auto &indexBuffer) {
            using BufferType = std::decay_t<decltype(indexBuffer)>;
            constexpr SDL_GPUIndexElementSize elementSize =
                    std::is_same_v<BufferType, Buffer<Uint16>>
                        ? SDL_GPU_INDEXELEMENTSIZE_16BIT
                        : SDL_GPU_INDEXELEMENTSIZE_32BIT;

            SDL_GPUBufferBinding indexBinding{};
            indexBinding.buffer = indexBuffer.Get();
            indexBinding.offset = 0;
            SDL_BindGPUIndexBuffer(renderPass, &indexBinding, elementSize);

            SDL_DrawGPUIndexedPrimitives(renderPass, indexBuffer.Count(), 1, 0, 0, 0);
        }, m_indexBuffer);
    }
}
