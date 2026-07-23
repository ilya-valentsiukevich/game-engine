//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/Vertex.h>

#include <SDL3/SDL_stdinc.h>
#include <filesystem>
#include <vector>

namespace Engine {
    // One glTF "primitive" — the smallest drawable unit in the format (one
    // material, one triangle list, see M5 §1.4) — already converted into
    // this engine's own Vertex layout and Uint16 index type, ready to hand
    // to Mesh's constructor unchanged.
    struct GltfPrimitive {
        std::vector<Vertex> vertices;
        std::vector<Uint16> indices;

        // Resolved relative to the .gltf file's own directory. Empty if the
        // primitive has no pbrMetallicRoughness.baseColorTexture.
        std::filesystem::path baseColorTexturePath;
    };

    // Parses a glTF 2.0 file (separate .gltf + .bin + textures form, see M5
    // §1.6) via cgltf and flattens every triangle-list primitive of every
    // mesh into a GltfPrimitive. Does not walk the scene graph (see M5
    // §1.4, "что дальше") — fine for a single static model at the origin.
    class GltfLoader {
    public:
        static std::vector<GltfPrimitive> Load(const std::filesystem::path &path);
    };
}
