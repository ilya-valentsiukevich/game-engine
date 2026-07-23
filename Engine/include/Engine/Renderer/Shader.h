//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>

#include <filesystem>

namespace Engine {
    // Owns a single compiled GPU shader module (one stage: vertex or fragment).
    class Shader {
    public:
        Shader(SDL_GPUDevice *device,
               const std::filesystem::path &path,
               SDL_GPUShaderStage stage);

        Shader(const Shader &) = delete;
        Shader &operator=(const Shader &) = delete;

        SDL_GPUShader *Get() const {
            return m_shader.Get();
        }

    private:
        GPUShaderHandle m_shader;
    };
}
