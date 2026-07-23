//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Texture.h>
#include <Engine/Renderer/Sampler.h>

namespace Engine {
    Material::Material(AssetHandle<Texture> baseColorTexture, const glm::vec4 &baseColorFactor,
                        const Sampler &sampler)
        : m_baseColorTexture(std::move(baseColorTexture)),
          m_baseColorFactor(baseColorFactor),
          m_sampler(&sampler) {
    }

    void Material::Bind(SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass) const {
        struct MaterialUniformBlock {
            glm::vec4 BaseColorFactor;
        };

        const MaterialUniformBlock materialUniform{m_baseColorFactor};
        SDL_PushGPUFragmentUniformData(commandBuffer, 1, &materialUniform, sizeof(materialUniform));

        SDL_GPUTextureSamplerBinding binding{};
        binding.texture = m_baseColorTexture->Get();
        binding.sampler = m_sampler->Get();

        SDL_BindGPUFragmentSamplers(renderPass, 0, &binding, 1);
    }
}
