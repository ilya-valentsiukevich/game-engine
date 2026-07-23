#pragma once

#include <Engine/Renderer/GPUResource.h>

namespace Engine {
    // Owns a 6-layer SDL_GPU_TEXTURETYPE_CUBE texture used as both a render
    // target (each face — and, for a multi-mip texture, each (face, mip)
    // pair — rendered into independently by EnvironmentMap's precompute
    // passes) and, once filled, an ordinary sampled cubemap.
    class CubemapTexture {
    public:
        CubemapTexture(SDL_GPUDevice *device, Uint32 size, Uint32 mipLevels,
                        SDL_GPUTextureFormat format);

        CubemapTexture(const CubemapTexture &) = delete;
        CubemapTexture &operator=(const CubemapTexture &) = delete;

        SDL_GPUTexture *Get() const { return m_texture.Get(); }
        Uint32 GetSize() const { return m_size; }
        Uint32 GetMipLevels() const { return m_mipLevels; }

    private:
        Uint32 m_size;
        Uint32 m_mipLevels;
        GPUTextureHandle m_texture;
    };
}
