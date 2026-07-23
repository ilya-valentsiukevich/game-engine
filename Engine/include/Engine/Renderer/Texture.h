//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>

#include <filesystem>

namespace Engine {
    // Owns a 2D GPU texture loaded from an image file (PNG/JPG/...) via
    // stb_image, uploaded once at construction through a transfer buffer —
    // same transfer-buffer-then-copy-pass pattern as Buffer<T> (M2), just
    // targeting a texture instead of a linear buffer.
    class Texture {
    public:
        Texture(SDL_GPUDevice *device, const std::filesystem::path &path);

        Texture(const Texture &) = delete;
        Texture &operator=(const Texture &) = delete;

        SDL_GPUTexture *Get() const {
            return m_texture.Get();
        }

    private:
        GPUTextureHandle m_texture;
    };
}
