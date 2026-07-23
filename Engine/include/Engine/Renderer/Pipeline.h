//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>

#include <optional>

namespace Engine {
    class Shader;

    // Owns a graphics pipeline built from a vertex and fragment Shader.
    class Pipeline {
    public:
        // Full: the existing (Position, Normal, TexCoord) mesh layout.
        // PositionOnly: a single float3 attribute — the skybox/precompute
        // unit cube. None: no vertex buffer at all — a fullscreen triangle
        // synthesized in the vertex shader from the vertex ID (BRDF LUT).
        enum class VertexLayout { Full, PositionOnly, None };

        // depthFormat == std::nullopt: no depth attachment at all (the
        // environment-map precompute passes, rendering into an empty
        // offscreen cubemap face with nothing to test against). Otherwise
        // depth-tested with the given compare op and write mode — the main
        // mesh pass wants LESS with writes on (the defaults below); the
        // skybox pass wants LESS_OR_EQUAL with writes off.
        Pipeline(SDL_GPUDevice *device,
                 SDL_GPUTextureFormat colorFormat,
                 std::optional<SDL_GPUTextureFormat> depthFormat,
                 const Shader &vertexShader,
                 const Shader &fragmentShader,
                 VertexLayout vertexLayout = VertexLayout::Full,
                 SDL_GPUCompareOp depthCompareOp = SDL_GPU_COMPAREOP_LESS,
                 bool enableDepthWrite = true);

        // Depth-only variant used for a shadow pass: no color target, same
        // vertex layout as the geometry pipeline above (a depth-only vertex
        // shader simply doesn't read the attributes it doesn't need), depth
        // bias enabled to fight shadow acne.
        Pipeline(SDL_GPUDevice *device,
                 SDL_GPUTextureFormat depthFormat,
                 const Shader &vertexShader,
                 const Shader &fragmentShader);

        Pipeline(const Pipeline &) = delete;
        Pipeline &operator=(const Pipeline &) = delete;

        SDL_GPUGraphicsPipeline *Get() const {
            return m_pipeline.Get();
        }

    private:
        GPUPipelineHandle m_pipeline;
    };
}
