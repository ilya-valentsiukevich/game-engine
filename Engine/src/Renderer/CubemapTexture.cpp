#include <Engine/Renderer/CubemapTexture.h>

#include <format>
#include <stdexcept>

namespace Engine {
    CubemapTexture::CubemapTexture(SDL_GPUDevice *device, Uint32 size, Uint32 mipLevels,
                                    SDL_GPUTextureFormat format)
        : m_size(size), m_mipLevels(mipLevels) {
        SDL_GPUTextureCreateInfo textureCreateInfo{};
        textureCreateInfo.type = SDL_GPU_TEXTURETYPE_CUBE;
        textureCreateInfo.format = format;
        // COLOR_TARGET: EnvironmentMap renders into each face directly, one
        // render pass per (face, mip) pair. SAMPLER: every later stage —
        // the next precompute pass, or Mesh.frag.msl/Skybox.frag.msl at
        // runtime — reads it back.
        textureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        textureCreateInfo.width = size;
        textureCreateInfo.height = size;
        textureCreateInfo.layer_count_or_depth = 6;
        textureCreateInfo.num_levels = mipLevels;
        textureCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

        m_texture = GPUTextureHandle(device, SDL_CreateGPUTexture(device, &textureCreateInfo));

        if (!m_texture) {
            throw std::runtime_error(
                std::format("Failed to create cubemap texture: {}", SDL_GetError()));
        }
    }
}
