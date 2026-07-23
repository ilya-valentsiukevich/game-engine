#include <Engine/Renderer/ShadowMap.h>

#include <format>
#include <stdexcept>

namespace Engine {
    ShadowMap::ShadowMap(SDL_GPUDevice *device, Uint32 size)
        : m_size(size) {
        SDL_GPUTextureCreateInfo textureCreateInfo{};
        textureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureCreateInfo.format = kFormat;
        // SAMPLER on top of DEPTH_STENCIL_TARGET: ShadowPass writes this
        // texture as a depth target, MainPass's fragment shader reads it
        // back as an ordinary texture in the same frame.
        textureCreateInfo.usage =
                SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        textureCreateInfo.width = size;
        textureCreateInfo.height = size;
        textureCreateInfo.layer_count_or_depth = 1;
        textureCreateInfo.num_levels = 1;
        textureCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

        SDL_GPUTexture *texture = SDL_CreateGPUTexture(device, &textureCreateInfo);

        if (!texture) {
            throw std::runtime_error(
                std::format("Failed to create shadow map texture: {}", SDL_GetError()));
        }

        m_texture = GPUTextureHandle(device, texture);

        SDL_GPUSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.min_filter = SDL_GPU_FILTER_LINEAR;
        samplerCreateInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
        samplerCreateInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        samplerCreateInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerCreateInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerCreateInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        // A comparison sampler: the shader calls sample_compare() instead
        // of sample(), the hardware runs the depth test per tap and
        // linearly filters the 0/1 results — one call already averages a
        // 2x2 neighborhood for free, before the manual PCF loop around it.
        samplerCreateInfo.enable_compare = true;
        samplerCreateInfo.compare_op = SDL_GPU_COMPAREOP_LESS;

        SDL_GPUSampler *sampler = SDL_CreateGPUSampler(device, &samplerCreateInfo);

        if (!sampler) {
            throw std::runtime_error(
                std::format("Failed to create shadow map sampler: {}", SDL_GetError()));
        }

        m_sampler = GPUSamplerHandle(device, sampler);
    }
}
