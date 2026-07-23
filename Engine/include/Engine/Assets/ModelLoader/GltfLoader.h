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
    // primitive of every mesh-holding node into a GltfPrimitive, with each
    // node's world transform already baked into its vertices — so a
    // multi-node file (e.g. a small scene authored in Blender) comes back
    // as one consistent set of primitives in a single shared space, not one
    // per node's own local mesh space.
    class GltfLoader {
    public:
        static std::vector<GltfPrimitive> Load(const std::filesystem::path &path);
    };
}
