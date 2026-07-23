//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Sampler.h>

#include <format>
#include <stdexcept>

namespace Engine {
    Sampler::Sampler(SDL_GPUDevice *device,
                      SDL_GPUFilter filter,
                      SDL_GPUSamplerAddressMode addressMode) {
        SDL_GPUSamplerCreateInfo createInfo{};
        createInfo.min_filter = filter;
        createInfo.mag_filter = filter;
        createInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        createInfo.address_mode_u = addressMode;
        createInfo.address_mode_v = addressMode;
        createInfo.address_mode_w = addressMode;

        // Textures now carry a full mip chain (see Texture::UploadPixels).
        // max_lod defaults to 0 when zero-initialized above, which would
        // clamp every lookup to mip 0 regardless of how many levels the
        // texture actually has — 1000 is the common "effectively
        // unclamped" convention (cf. Vulkan's VK_LOD_CLAMP_NONE); the
        // texture's real level count always caps it below that anyway.
        createInfo.max_lod = 1000.0f;

        createInfo.enable_anisotropy = true;
        createInfo.max_anisotropy = 16.0f; // hardware clamps to its own max if lower

        m_sampler = GPUSamplerHandle(device, SDL_CreateGPUSampler(device, &createInfo));

        if (!m_sampler) {
            throw std::runtime_error(
                std::format("Failed to create sampler: {}", SDL_GetError()));
        }
    }
}
