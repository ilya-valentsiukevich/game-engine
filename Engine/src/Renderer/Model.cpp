//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Model.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Texture.h>
#include <Engine/Assets/AssetManager.h>
#include <Engine/Assets/ModelLoader/GltfLoader.h>

#include <format>
#include <span>
#include <stdexcept>

namespace Engine {
    namespace {
        AssetHandle<Texture> LoadBaseColorTexture(
            SDL_GPUDevice *device, const GltfPrimitive &primitive,
            AssetCache<Texture> &textureCache) {
            if (!primitive.baseColorTexturePath.empty()) {
                return textureCache.Load(
                    primitive.baseColorTexturePath,
                    [device, path = primitive.baseColorTexturePath] {
                        return std::make_shared<Texture>(device, path);
                    },
                    [device](Texture &texture, const std::filesystem::path &path) {
                        texture.Reload(device, path);
                    });
            }

            if (!primitive.baseColorTextureData.empty()) {
                // No stable file path to key a cache entry (or a hot-reload)
                // on — embedded data gets its own uncached Texture.
                return std::make_shared<Texture>(
                    device, std::span(primitive.baseColorTextureData));
            }

            return nullptr;
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

            AssetHandle<Texture> baseColorTexture =
                    LoadBaseColorTexture(device, primitive, assets.Textures);

            if (!baseColorTexture) {
                throw std::runtime_error(std::format(
                    "Primitive in model ({}) has no base color texture", path.string()));
            }

            part.material = std::make_unique<Material>(std::move(baseColorTexture), sampler);

            m_parts.push_back(std::move(part));
        }
    }

    Model::~Model() = default;

    void Model::Draw(SDL_GPURenderPass *renderPass) const {
        for (const MeshPart &part : m_parts) {
            part.material->Bind(renderPass);
            part.mesh->Draw(renderPass);
        }
    }
}
