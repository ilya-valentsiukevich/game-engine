//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Texture.h>
#include <Engine/Renderer/Sampler.h>

namespace Engine {
    Material::Material(AssetHandle<Texture> baseColorTexture, const glm::vec4 &baseColorFactor,
                        AssetHandle<Texture> metallicRoughnessTexture,
                        float metallicFactor, float roughnessFactor,
                        const Sampler &sampler)
        : m_baseColorTexture(std::move(baseColorTexture)),
          m_baseColorFactor(baseColorFactor),
          m_metallicRoughnessTexture(std::move(metallicRoughnessTexture)),
          m_metallicFactor(metallicFactor),
          m_roughnessFactor(roughnessFactor),
          m_sampler(&sampler) {
    }

    void Material::Bind(SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass) const {
        struct MaterialUniformBlock {
            glm::vec4 BaseColorFactor;
            glm::vec4 Params; // x: metallic factor, y: roughness factor, z/w: unused
        };

        const MaterialUniformBlock materialUniform{
            m_baseColorFactor, glm::vec4(m_metallicFactor, m_roughnessFactor, 0.0f, 0.0f)};
        SDL_PushGPUFragmentUniformData(commandBuffer, 1, &materialUniform, sizeof(materialUniform));

        SDL_GPUTextureSamplerBinding bindings[2]{};
        bindings[0].texture = m_baseColorTexture->Get();
        bindings[0].sampler = m_sampler->Get();
        bindings[1].texture = m_metallicRoughnessTexture->Get();
        bindings[1].sampler = m_sampler->Get();

        SDL_BindGPUFragmentSamplers(renderPass, 0, bindings, 2);
    }
}
