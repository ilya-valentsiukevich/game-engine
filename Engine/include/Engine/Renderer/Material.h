//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Assets/AssetCache.h>
#include <Engine/Renderer/GlmConfig.h>
#include <Engine/Renderer/GPUResource.h>

#include <glm/glm.hpp>

namespace Engine {
    class Texture;
    class Sampler;

    // Pairs a base-color texture handle with a shared, non-owning Sampler
    // and a baseColorFactor tint. The texture is a shared AssetHandle
    // rather than an owned value so several Materials — and, after a
    // hot-reload, every draw already using this Material — point at the
    // same underlying Texture.
    class Material {
    public:
        Material(AssetHandle<Texture> baseColorTexture, const glm::vec4 &baseColorFactor,
                  const Sampler &sampler);

        Material(const Material &) = delete;
        Material &operator=(const Material &) = delete;

        void Bind(SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass) const;

    private:
        AssetHandle<Texture> m_baseColorTexture;
        glm::vec4 m_baseColorFactor;
        const Sampler *m_sampler;
    };
}
