//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>

namespace Engine {
    class Shader;

    // Owns a graphics pipeline built from a vertex and fragment Shader.
    class Pipeline {
    public:
        Pipeline(SDL_GPUDevice *device,
                 SDL_GPUTextureFormat colorFormat,
                 SDL_GPUTextureFormat depthFormat,
                 const Shader &vertexShader,
                 const Shader &fragmentShader);

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
