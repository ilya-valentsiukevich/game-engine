//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Shader.h>
#include <Engine/Assets/ShaderLoader/ShaderLoader.h>

#include <format>
#include <stdexcept>

namespace Engine {
    Shader::Shader(SDL_GPUDevice *device,
                   const std::filesystem::path &path,
                   SDL_GPUShaderStage stage) {
        const auto code = ShaderLoader::LoadBinary(path);

        SDL_GPUShaderCreateInfo createInfo{};
        createInfo.code = reinterpret_cast<const Uint8 *>(code.data());
        createInfo.code_size = code.size();
        createInfo.entrypoint = "main0";
        createInfo.format = SDL_GPU_SHADERFORMAT_MSL;
        createInfo.stage = stage;
        createInfo.num_samplers = 0;
        createInfo.num_uniform_buffers = 0;
        createInfo.num_storage_buffers = 0;
        createInfo.num_storage_textures = 0;

        SDL_GPUShader *shader = SDL_CreateGPUShader(device, &createInfo);

        if (!shader) {
            throw std::runtime_error(
                std::format("Failed to create shader ({}): {}",
                    path.string(), SDL_GetError()));
        }

        m_shader = GPUShaderHandle(device, shader);
    }
}
