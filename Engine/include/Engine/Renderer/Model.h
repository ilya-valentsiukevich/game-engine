//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

namespace Engine {
    class Mesh;
    class Material;
    class Sampler;
    struct AssetManager;

    // A model loaded from a glTF file: one (Mesh, Material) pair per glTF
    // primitive, as a flat list — no scene graph/node hierarchy.
    class Model {
    public:
        Model(SDL_GPUDevice *device,
              const std::filesystem::path &path,
              const Sampler &sampler,
              AssetManager &assets);
        ~Model();

        Model(const Model &) = delete;
        Model &operator=(const Model &) = delete;

        void Draw(SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass) const;

        // Draws every part's geometry only, skipping material/texture
        // binding — for depth-only passes, where the bound pipeline has no
        // fragment-stage resources to bind them to anyway.
        void DrawDepthOnly(SDL_GPURenderPass *renderPass) const;

        // For DebugUI's material inspector, which shows one editable block
        // per part — a model can have several (e.g. body + separate cape
        // material), each with its own Material to edit independently.
        std::size_t GetPartCount() const { return m_parts.size(); }
        Material &GetMaterial(std::size_t index) const { return *m_parts[index].material; }

    private:
        // ~Model() must be defined in Model.cpp (not defaulted here): it
        // implicitly destroys every MeshPart's unique_ptr<Mesh>/<Material>,
        // which needs their complete types — only forward-declared above.
        struct MeshPart {
            std::unique_ptr<Mesh> mesh;
            std::unique_ptr<Material> material;
        };

        std::vector<MeshPart> m_parts;
    };
}
