//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>
#include <Engine/Renderer/Texture.h>

#include <filesystem>
#include <span>

namespace Engine {
    class Sampler;

    // Pairs a base-color texture with a shared, non-owning Sampler.
    class Material {
    public:
        // Base color image is an external file.
        Material(SDL_GPUDevice *device,
                  const std::filesystem::path &baseColorTexturePath,
                  const Sampler &sampler);

        // Base color image is already in memory (e.g. embedded in a .glb).
        Material(SDL_GPUDevice *device,
                  std::span<const unsigned char> baseColorTextureData,
                  const Sampler &sampler);

        Material(const Material &) = delete;
        Material &operator=(const Material &) = delete;

        void Bind(SDL_GPURenderPass *renderPass) const;

    private:
        Texture m_baseColorTexture;
        const Sampler *m_sampler;
    };
}
