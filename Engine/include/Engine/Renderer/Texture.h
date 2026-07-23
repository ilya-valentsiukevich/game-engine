//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>

#include <filesystem>
#include <span>

namespace Engine {
    // Owns a 2D GPU texture decoded from an encoded image (PNG/JPG/...) via
    // stb_image, uploaded once at construction through a transfer buffer —
    // same transfer-buffer-then-copy-pass pattern as Buffer<T>, just
    // targeting a texture instead of a linear buffer.
    class Texture {
    public:
        // format decides how the GPU interprets the uploaded bytes. Color
        // data (base color/albedo) needs an _SRGB format so the hardware
        // gamma-decodes it to linear on sample; data that isn't color
        // (normal maps, metallic/roughness/occlusion) is already linear and
        // must use a plain UNORM format instead — sampling it as _SRGB would
        // silently darken it. There's no safe default, so every call site
        // must say which one it means.

        // Loads and decodes the image from a file on disk.
        Texture(SDL_GPUDevice *device, const std::filesystem::path &path, SDL_GPUTextureFormat format);

        // Decodes an image that's already in memory (e.g. embedded in a
        // .glb's binary chunk) instead of reading it from a file.
        Texture(SDL_GPUDevice *device, std::span<const unsigned char> encodedImageData,
                SDL_GPUTextureFormat format);

        Texture(const Texture &) = delete;
        Texture &operator=(const Texture &) = delete;

        // Re-decodes the same file and replaces the GPU texture in place —
        // existing AssetHandle<Texture> holders keep the same Texture
        // object, so they pick up the new pixels on their very next Get().
        // Only makes sense for a texture originally loaded from a file, not
        // from in-memory encoded bytes (the other constructor above). Reuses
        // the format passed to the constructor — a texture's color space
        // doesn't change across reloads.
        void Reload(SDL_GPUDevice *device, const std::filesystem::path &path);

        SDL_GPUTexture *Get() const {
            return m_texture.Get();
        }

    private:
        // Takes ownership of pixels (an stb_image allocation) and frees it
        // via stbi_image_free() on every path, success or throw.
        void UploadPixels(SDL_GPUDevice *device, unsigned char *pixels, int width, int height);

        SDL_GPUTextureFormat m_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        GPUTextureHandle m_texture;
    };
}
