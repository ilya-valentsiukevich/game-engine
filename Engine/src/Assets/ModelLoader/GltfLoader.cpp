//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Assets/ModelLoader/GltfLoader.h>
#include <Engine/Renderer/GlmConfig.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <format>
#include <stdexcept>

namespace Engine {
    namespace {
        // cgltf_data is a plain C struct — cgltf_free() must run even if
        // something below throws before Load() returns normally.
        struct CgltfDataGuard {
            cgltf_data *data = nullptr;

            ~CgltfDataGuard() {
                if (data)
                    cgltf_free(data);
            }
        };

        // glTF URIs are percent-encoded (RFC 3986): a filename with a space
        // becomes "%20" in the JSON, which won't match any file on disk
        // unless decoded first.
        std::string DecodeUri(std::string_view uri) {
            std::string result;
            result.reserve(uri.size());

            for (size_t i = 0; i < uri.size(); ++i) {
                if (uri[i] == '%' && i + 2 < uri.size()) {
                    const int value =
                            std::stoi(std::string(uri.substr(i + 1, 2)), nullptr, 16);
                    result.push_back(static_cast<char>(value));
                    i += 2;
                } else {
                    result.push_back(uri[i]);
                }
            }

            return result;
        }

        void FillBaseColorTexture(
            const cgltf_primitive &primitive, const std::filesystem::path &modelDir,
            GltfPrimitive &out) {
            const cgltf_material *material = primitive.material;

            if (!material || !material->has_pbr_metallic_roughness)
                return;

            const cgltf_texture *texture =
                    material->pbr_metallic_roughness.base_color_texture.texture;

            if (!texture || !texture->image)
                return;

            const cgltf_image &image = *texture->image;

            if (image.uri) {
                // Assumes a relative file path; a data:-URI (inline base64)
                // isn't handled here.
                out.baseColorTexturePath = modelDir / DecodeUri(image.uri);
            } else if (image.buffer_view) {
                const uint8_t *data = cgltf_buffer_view_data(image.buffer_view);
                const cgltf_size size = image.buffer_view->size;
                out.baseColorTextureData.assign(data, data + size);
            }
        }

        // Folds the node's world transform (its own TRS composed with every
        // ancestor's, via cgltf) into the primitive's vertices, so the
        // returned geometry sits in the same space regardless of where in
        // the node hierarchy it came from — a mesh nested under a moved/
        // rotated/scaled node (any non-trivial Blender export) ends up
        // exactly where the artist placed it instead of at its raw local
        // origin.
        void BakeNodeTransform(const cgltf_node &node, GltfPrimitive &primitive) {
            cgltf_float worldMatrix[16];
            cgltf_node_transform_world(&node, worldMatrix);

            const glm::mat4 model = glm::make_mat4(worldMatrix);
            const glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(model));

            for (Vertex &vertex : primitive.vertices) {
                const glm::vec3 position = glm::vec3(
                    model * glm::vec4(vertex.Position[0], vertex.Position[1], vertex.Position[2], 1.0f));
                vertex.Position[0] = position.x;
                vertex.Position[1] = position.y;
                vertex.Position[2] = position.z;

                const glm::vec3 normal = glm::normalize(normalMatrix *
                    glm::vec3(vertex.Normal[0], vertex.Normal[1], vertex.Normal[2]));
                vertex.Normal[0] = normal.x;
                vertex.Normal[1] = normal.y;
                vertex.Normal[2] = normal.z;
            }
        }
    }

    std::vector<GltfPrimitive> GltfLoader::Load(const std::filesystem::path &path) {
        cgltf_options options{};
        CgltfDataGuard guard;

        cgltf_result result = cgltf_parse_file(&options, path.string().c_str(), &guard.data);

        if (result != cgltf_result_success) {
            throw std::runtime_error(std::format(
                "Failed to parse glTF ({}): cgltf_result {}",
                path.string(), static_cast<int>(result)));
        }

        result = cgltf_load_buffers(&options, guard.data, path.string().c_str());

        if (result != cgltf_result_success) {
            throw std::runtime_error(std::format(
                "Failed to load glTF buffers ({}): cgltf_result {}",
                path.string(), static_cast<int>(result)));
        }

        const std::filesystem::path modelDir = path.parent_path();
        std::vector<GltfPrimitive> primitives;

        // Walking the flat node list (rather than data->scene->nodes
        // recursively) gives the same result: cgltf_node_transform_world
        // composes a node's full ancestor chain internally regardless of
        // how it's reached, so there's no need to re-walk the hierarchy by
        // hand here. Nodes without a mesh (empty transform/joint/camera
        // nodes) are simply skipped.
        for (cgltf_size nodeIndex = 0; nodeIndex < guard.data->nodes_count; ++nodeIndex) {
            const cgltf_node &node = guard.data->nodes[nodeIndex];

            if (!node.mesh)
                continue;

            const cgltf_mesh &mesh = *node.mesh;

            for (cgltf_size primIndex = 0; primIndex < mesh.primitives_count; ++primIndex) {
                const cgltf_primitive &primitive = mesh.primitives[primIndex];

                if (primitive.type != cgltf_primitive_type_triangles)
                    continue;

                const cgltf_accessor *positionAccessor =
                        cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
                const cgltf_accessor *normalAccessor =
                        cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0);
                const cgltf_accessor *texCoordAccessor =
                        cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0);

                if (!positionAccessor) {
                    throw std::runtime_error(std::format(
                        "glTF primitive has no POSITION attribute ({})", path.string()));
                }

                if (!normalAccessor) {
                    throw std::runtime_error(std::format(
                        "glTF primitive has no NORMAL attribute ({}) — lighting needs "
                        "per-vertex normals, re-export the model with normals included",
                        path.string()));
                }

                if (!primitive.indices) {
                    throw std::runtime_error(std::format(
                        "glTF primitive has no index buffer ({}) — non-indexed "
                        "primitives aren't supported", path.string()));
                }

                GltfPrimitive out;
                out.vertices.resize(positionAccessor->count);

                for (cgltf_size i = 0; i < positionAccessor->count; ++i) {
                    if (!cgltf_accessor_read_float(positionAccessor, i, out.vertices[i].Position, 3)) {
                        throw std::runtime_error(std::format(
                            "Failed to read POSITION[{}] ({})", i, path.string()));
                    }

                    if (!cgltf_accessor_read_float(normalAccessor, i, out.vertices[i].Normal, 3)) {
                        throw std::runtime_error(std::format(
                            "Failed to read NORMAL[{}] ({})", i, path.string()));
                    }

                    if (texCoordAccessor) {
                        cgltf_accessor_read_float(texCoordAccessor, i, out.vertices[i].TexCoord, 2);
                    } else {
                        out.vertices[i].TexCoord[0] = 0.0f;
                        out.vertices[i].TexCoord[1] = 0.0f;
                    }
                }

                out.indices.reserve(primitive.indices->count);

                for (cgltf_size i = 0; i < primitive.indices->count; ++i) {
                    const cgltf_size index = cgltf_accessor_read_index(primitive.indices, i);

                    if (index > 0xFFFFu) {
                        throw std::runtime_error(std::format(
                            "glTF vertex index {} doesn't fit Uint16 ({})",
                            index, path.string()));
                    }

                    out.indices.push_back(static_cast<Uint16>(index));
                }

                FillBaseColorTexture(primitive, modelDir, out);
                BakeNodeTransform(node, out);

                primitives.push_back(std::move(out));
            }
        }

        return primitives;
    }
}
