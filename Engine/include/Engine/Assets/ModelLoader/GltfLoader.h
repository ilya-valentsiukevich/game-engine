//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/Vertex.h>

#include <SDL3/SDL_stdinc.h>
#include <filesystem>
#include <vector>

namespace Engine {
    // One glTF primitive: one material, one triangle list, already
    // converted into this engine's own Vertex layout (position, normal,
    // UV) and Uint16 indices.
    struct GltfPrimitive {
        std::vector<Vertex> vertices;
        std::vector<Uint16> indices;

        // At most one of these is set: the base color image is either an
        // external file (baseColorTexturePath) or embedded in the glTF/GLB
        // binary chunk (baseColorTextureData). Both empty means the
        // primitive has no base color texture.
        std::filesystem::path baseColorTexturePath;
        std::vector<unsigned char> baseColorTextureData;
    };

    // Parses a glTF 2.0 file via cgltf and flattens every triangle-list
    // primitive of every mesh into a GltfPrimitive. Ignores node transforms
    // — every primitive comes back in its own local mesh space.
    class GltfLoader {
    public:
        static std::vector<GltfPrimitive> Load(const std::filesystem::path &path);
    };
}
