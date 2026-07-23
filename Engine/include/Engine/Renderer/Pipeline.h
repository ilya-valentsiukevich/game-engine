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
