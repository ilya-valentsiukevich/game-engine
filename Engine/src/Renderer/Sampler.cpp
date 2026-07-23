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
        createInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        createInfo.address_mode_u = addressMode;
        createInfo.address_mode_v = addressMode;
        createInfo.address_mode_w = addressMode;

        m_sampler = GPUSamplerHandle(device, SDL_CreateGPUSampler(device, &createInfo));

        if (!m_sampler) {
            throw std::runtime_error(
                std::format("Failed to create sampler: {}", SDL_GetError()));
        }
    }
}
