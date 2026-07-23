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
        // Loads and decodes the image from a file on disk.
        Texture(SDL_GPUDevice *device, const std::filesystem::path &path);

        // Decodes an image that's already in memory (e.g. embedded in a
        // .glb's binary chunk) instead of reading it from a file.
        Texture(SDL_GPUDevice *device, std::span<const unsigned char> encodedImageData);

        Texture(const Texture &) = delete;
        Texture &operator=(const Texture &) = delete;

        SDL_GPUTexture *Get() const {
            return m_texture.Get();
        }

    private:
        // Takes ownership of pixels (an stb_image allocation) and frees it
        // via stbi_image_free() on every path, success or throw.
        void UploadPixels(SDL_GPUDevice *device, unsigned char *pixels, int width, int height);

        GPUTextureHandle m_texture;
    };
}
