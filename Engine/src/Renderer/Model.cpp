//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Model.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Texture.h>
#include <Engine/Renderer/GlmConfig.h>
#include <Engine/Assets/AssetManager.h>
#include <Engine/Assets/ModelLoader/GltfLoader.h>

#include <glm/glm.hpp>

#include <span>

namespace Engine {
    namespace {
        AssetHandle<Texture> LoadBaseColorTexture(
            SDL_GPUDevice *device, const GltfPrimitive &primitive, AssetManager &assets) {
            // Base color is the texture the fragment shader multiplies
            // directly into the lit color (see Mesh.frag.msl) — it's color
            // data, so it needs an _SRGB format for the hardware to decode
            // it to linear on sample.
            constexpr SDL_GPUTextureFormat kBaseColorFormat = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;

            if (!primitive.baseColorTexturePath.empty()) {
                return assets.Textures.Load(
                    primitive.baseColorTexturePath,
                    [device, path = primitive.baseColorTexturePath, kBaseColorFormat] {
                        return std::make_shared<Texture>(device, path, kBaseColorFormat);
                    },
                    [device](Texture &texture, const std::filesystem::path &path) {
                        texture.Reload(device, path);
                    });
            }

            if (!primitive.baseColorTextureData.empty()) {
                // No stable file path to key a cache entry (or a hot-reload)
                // on — embedded data gets its own uncached Texture.
                return std::make_shared<Texture>(
                    device, std::span(primitive.baseColorTextureData), kBaseColorFormat);
            }

            // No texture at all (common for solid-colored props/placeholder
            // assets): fall back to a shared white texture so baseColorFactor
            // alone still determines the color, instead of treating this as
            // an error.
            return assets.GetWhiteTexture(device);
        }

        AssetHandle<Texture> LoadMetallicRoughnessTexture(
            SDL_GPUDevice *device, const GltfPrimitive &primitive, AssetManager &assets) {
            // Metallic/roughness values aren't color data — they must stay
            // UNORM, not _SRGB, or the hardware's gamma decode on sample
            // would silently distort every mid-range value.
            constexpr SDL_GPUTextureFormat kMetallicRoughnessFormat = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

            if (!primitive.metallicRoughnessTexturePath.empty()) {
                return assets.Textures.Load(
                    primitive.metallicRoughnessTexturePath,
                    [device, path = primitive.metallicRoughnessTexturePath, kMetallicRoughnessFormat] {
                        return std::make_shared<Texture>(device, path, kMetallicRoughnessFormat);
                    },
                    [device](Texture &texture, const std::filesystem::path &path) {
                        texture.Reload(device, path);
                    });
            }

            if (!primitive.metallicRoughnessTextureData.empty()) {
                return std::make_shared<Texture>(
                    device, std::span(primitive.metallicRoughnessTextureData), kMetallicRoughnessFormat);
            }

            // No texture: assets.GetWhiteTexture's stored bytes are all
            // 255, which sample to 1.0 whether interpreted as _SRGB or
            // UNORM (white is a fixed point of the sRGB transfer function)
            // — reusing the same cached white texture here is exact, not
            // an approximation, so the material's metallic/roughness
            // factors alone decide the result.
            return assets.GetWhiteTexture(device);
        }
    }

    Model::Model(SDL_GPUDevice *device,
                 const std::filesystem::path &path,
                 const Sampler &sampler,
                 AssetManager &assets) {
        const std::vector<GltfPrimitive> primitives = GltfLoader::Load(path);

        m_parts.reserve(primitives.size());

        for (const GltfPrimitive &primitive : primitives) {
            MeshPart part;
            part.mesh = std::make_unique<Mesh>(
                device, std::span(primitive.vertices), std::span(primitive.indices));

            AssetHandle<Texture> baseColorTexture = LoadBaseColorTexture(device, primitive, assets);
            AssetHandle<Texture> metallicRoughnessTexture =
                    LoadMetallicRoughnessTexture(device, primitive, assets);

            const glm::vec4 baseColorFactor(
                primitive.baseColorFactor[0], primitive.baseColorFactor[1],
                primitive.baseColorFactor[2], primitive.baseColorFactor[3]);

            part.material = std::make_unique<Material>(
                std::move(baseColorTexture), baseColorFactor,
                std::move(metallicRoughnessTexture), primitive.metallicFactor, primitive.roughnessFactor,
                sampler);

            m_parts.push_back(std::move(part));
        }
    }

    Model::~Model() = default;

    void Model::Draw(SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass) const {
        for (const MeshPart &part : m_parts) {
            part.material->Bind(commandBuffer, renderPass);
            part.mesh->Draw(renderPass);
        }
    }

    void Model::DrawDepthOnly(SDL_GPURenderPass *renderPass) const {
        for (const MeshPart &part : m_parts) {
            part.mesh->Draw(renderPass);
        }
    }
}
