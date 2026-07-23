#pragma once

#include <Engine/Renderer/GPUResource.h>

namespace Engine {
    // Depth-only render target that ShadowPass writes and MainPass's
    // fragment shader samples back. Fixed resolution, independent of the
    // window (unlike Renderer's own m_depthTexture, which is recreated on
    // resize) — shadow sharpness is a quality knob, not tied to display
    // size.
    class ShadowMap {
    public:
        static constexpr SDL_GPUTextureFormat kFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

        ShadowMap(SDL_GPUDevice *device, Uint32 size);

        ShadowMap(const ShadowMap &) = delete;
        ShadowMap &operator=(const ShadowMap &) = delete;

        SDL_GPUTexture *GetTexture() const { return m_texture.Get(); }
        SDL_GPUSampler *GetSampler() const { return m_sampler.Get(); }
        Uint32 GetSize() const { return m_size; }

    private:
        Uint32 m_size;
        GPUTextureHandle m_texture;
        GPUSamplerHandle m_sampler;
    };
}
