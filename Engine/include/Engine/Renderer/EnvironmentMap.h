#pragma once

#include <Engine/Renderer/GPUResource.h>
#include <Engine/Renderer/GlmConfig.h>
#include <Engine/Renderer/Buffer.h>
#include <Engine/Renderer/CubemapTexture.h>

#include <glm/glm.hpp>

#include <filesystem>
#include <memory>

namespace Engine {
    // Loads an HDRI equirectangular panorama once at construction and bakes
    // everything M14's IBL needs from it: a sampleable environment cubemap
    // (the skybox background itself), a small diffuse irradiance cubemap, a
    // roughness-mipmapped specular prefiltered cubemap, and a BRDF
    // integration LUT. All four are baked here, once, and never touched
    // again per frame — see docs/M14-skybox-ibl.md theory §1.9 for why this
    // uses a rasterizer loop (6, or 6 x mip levels, render passes) rather
    // than a compute shader the engine doesn't have.
    class EnvironmentMap {
    public:
        static constexpr Uint32 kEnvironmentSize = 512;
        static constexpr Uint32 kIrradianceSize = 32;
        static constexpr Uint32 kPrefilterSize = 128;
        static constexpr Uint32 kPrefilterMipLevels = 5;
        static constexpr Uint32 kBRDFLutSize = 512;
        static constexpr SDL_GPUTextureFormat kHdrFormat = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        static constexpr SDL_GPUTextureFormat kBRDFLutFormat = SDL_GPU_TEXTUREFORMAT_R16G16_FLOAT;

        EnvironmentMap(SDL_GPUDevice *device, const std::filesystem::path &hdrPath);

        EnvironmentMap(const EnvironmentMap &) = delete;
        EnvironmentMap &operator=(const EnvironmentMap &) = delete;

        // Issues the 36-vertex, non-indexed draw call for the shared unit
        // cube. Used internally by the precompute passes below and by
        // Renderer's runtime skybox pass — the geometry and its GPU buffer
        // exist exactly once either way.
        void DrawCube(SDL_GPURenderPass *renderPass) const;

        SDL_GPUTexture *GetSkyboxTexture() const { return m_environmentCubemap->Get(); }
        SDL_GPUTexture *GetIrradianceTexture() const { return m_irradianceCubemap->Get(); }
        SDL_GPUTexture *GetPrefilteredTexture() const { return m_prefilteredCubemap->Get(); }
        SDL_GPUTexture *GetBRDFLutTexture() const { return m_brdfLutTexture.Get(); }
        SDL_GPUSampler *GetHdrSampler() const { return m_hdrSampler.Get(); }

    private:
        std::unique_ptr<CubemapTexture> m_environmentCubemap;
        std::unique_ptr<CubemapTexture> m_irradianceCubemap;
        std::unique_ptr<CubemapTexture> m_prefilteredCubemap;
        GPUTextureHandle m_brdfLutTexture;
        GPUSamplerHandle m_hdrSampler;

        Buffer<glm::vec3> m_cubeVertexBuffer;
    };
}
