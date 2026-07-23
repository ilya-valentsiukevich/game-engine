//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>

namespace Engine {
    // Thin RAII wrapper for an SDL_GPUSampler: filtering + addressing rules
    // applied when a shader reads a texture. Deliberately separate from
    // Texture — the same sampler could be reused across many textures, even
    // though the engine only needs one of each right now.
    class Sampler {
    public:
        Sampler(SDL_GPUDevice *device,
                SDL_GPUFilter filter,
                SDL_GPUSamplerAddressMode addressMode);

        Sampler(const Sampler &) = delete;
        Sampler &operator=(const Sampler &) = delete;

        SDL_GPUSampler *Get() const {
            return m_sampler.Get();
        }

    private:
        GPUSamplerHandle m_sampler;
    };
}
