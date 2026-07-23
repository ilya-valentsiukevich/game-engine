//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Assets/ModelLoader/GltfLoader.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

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

        for (cgltf_size meshIndex = 0; meshIndex < guard.data->meshes_count; ++meshIndex) {
            const cgltf_mesh &mesh = guard.data->meshes[meshIndex];

            for (cgltf_size primIndex = 0; primIndex < mesh.primitives_count; ++primIndex) {
                const cgltf_primitive &primitive = mesh.primitives[primIndex];

                if (primitive.type != cgltf_primitive_type_triangles)
                    continue;

                const cgltf_accessor *positionAccessor =
                        cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
                const cgltf_accessor *texCoordAccessor =
                        cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0);

                if (!positionAccessor) {
                    throw std::runtime_error(std::format(
                        "glTF primitive has no POSITION attribute ({})", path.string()));
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

                primitives.push_back(std::move(out));
            }
        }

        return primitives;
    }
}
