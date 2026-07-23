//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace Engine {
    class Mesh;
    class Material;
    class Sampler;

    // A model loaded from a glTF file: one (Mesh, Material) pair per glTF
    // primitive (see M5 §1.4). Flat list, no scene graph/node hierarchy —
    // fine for a single static object at the origin, the scope of M5.
    class Model {
    public:
        Model(SDL_GPUDevice *device,
              const std::filesystem::path &path,
              const Sampler &sampler);

        Model(const Model &) = delete;
        Model &operator=(const Model &) = delete;

        void Draw(SDL_GPURenderPass *renderPass) const;

    private:
        struct MeshPart {
            std::unique_ptr<Mesh> mesh;
            std::unique_ptr<Material> material;
        };

        std::vector<MeshPart> m_parts;
    };
}
