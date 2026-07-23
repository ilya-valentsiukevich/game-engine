//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Sampler.h>

namespace Engine {
    Material::Material(SDL_GPUDevice *device,
                        const std::filesystem::path &baseColorTexturePath,
                        const Sampler &sampler)
        : m_baseColorTexture(device, baseColorTexturePath), m_sampler(&sampler) {
    }

    void Material::Bind(SDL_GPURenderPass *renderPass) const {
        SDL_GPUTextureSamplerBinding binding{};
        binding.texture = m_baseColorTexture.Get();
        binding.sampler = m_sampler->Get();

        SDL_BindGPUFragmentSamplers(renderPass, 0, &binding, 1);
    }
}
