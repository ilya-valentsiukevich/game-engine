//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>
#include <Engine/Renderer/Texture.h>

#include <filesystem>

namespace Engine {
    class Sampler;

    // Pairs a base-color Texture with the (shared, non-owning) Sampler used
    // to read it — glTF's PBR metallic-roughness material has several more
    // texture slots (metallic-roughness, normal, occlusion, emissive), all
    // ignored for now since nothing in the engine shades with them yet
    // (see M5 §1.4 and "что дальше").
    class Material {
    public:
        Material(SDL_GPUDevice *device,
                  const std::filesystem::path &baseColorTexturePath,
                  const Sampler &sampler);

        Material(const Material &) = delete;
        Material &operator=(const Material &) = delete;

        void Bind(SDL_GPURenderPass *renderPass) const;

    private:
        Texture m_baseColorTexture;
        const Sampler *m_sampler;
    };
}
