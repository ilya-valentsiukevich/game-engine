//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Model.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Assets/ModelLoader/GltfLoader.h>

#include <format>
#include <span>
#include <stdexcept>

namespace Engine {
    Model::Model(SDL_GPUDevice *device,
                 const std::filesystem::path &path,
                 const Sampler &sampler) {
        const std::vector<GltfPrimitive> primitives = GltfLoader::Load(path);

        m_parts.reserve(primitives.size());

        for (const GltfPrimitive &primitive : primitives) {
            if (primitive.baseColorTexturePath.empty()) {
                throw std::runtime_error(std::format(
                    "Primitive in model ({}) has no base color texture — see M5 "
                    "§1.6 (Blender: Image Texture node in Base Color)", path.string()));
            }

            MeshPart part;
            part.mesh = std::make_unique<Mesh>(
                device, std::span(primitive.vertices), std::span(primitive.indices));
            part.material = std::make_unique<Material>(
                device, primitive.baseColorTexturePath, sampler);

            m_parts.push_back(std::move(part));
        }
    }

    void Model::Draw(SDL_GPURenderPass *renderPass) const {
        for (const MeshPart &part : m_parts) {
            part.material->Bind(renderPass);
            part.mesh->Draw(renderPass);
        }
    }
}
