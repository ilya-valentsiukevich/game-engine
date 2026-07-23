//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Texture.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>
#include <format>
#include <stdexcept>

namespace Engine {
    namespace {
        // stb_image's decoded buffer must be freed via stbi_image_free() no
        // matter how the enclosing scope exits — same "free even on throw"
        // need as GltfLoader's CgltfDataGuard.
        struct StbImageGuard {
            stbi_uc *pixels = nullptr;

            ~StbImageGuard() {
                if (pixels)
                    stbi_image_free(pixels);
            }
        };
    }

    Texture::Texture(SDL_GPUDevice *device, const std::filesystem::path &path, SDL_GPUTextureFormat format)
        : m_format(format) {
        Reload(device, path);
    }

    Texture::Texture(SDL_GPUDevice *device, std::span<const unsigned char> encodedImageData,
                      SDL_GPUTextureFormat format)
        : m_format(format) {
        int width = 0;
        int height = 0;
        int sourceChannels = 0;

        StbImageGuard guard;
        guard.pixels = stbi_load_from_memory(
            encodedImageData.data(), static_cast<int>(encodedImageData.size()),
            &width, &height, &sourceChannels, 4);

        if (!guard.pixels) {
            throw std::runtime_error(
                std::format("Failed to decode embedded texture: {}", stbi_failure_reason()));
        }

        UploadPixels(device, guard.pixels, width, height);
    }

    Texture::Texture(SDL_GPUDevice *device, const std::array<unsigned char, 4> &rgba, SDL_GPUTextureFormat format)
        : m_format(format) {
        UploadPixels(device, rgba.data(), 1, 1);
    }

    void Texture::Reload(SDL_GPUDevice *device, const std::filesystem::path &path) {
        int width = 0;
        int height = 0;
        int sourceChannels = 0;

        // Force 4 channels (RGBA) regardless of the source file's actual
        // channel count, since the tightly-packed-4-bytes-per-pixel upload
        // below expects it.
        StbImageGuard guard;
        guard.pixels = stbi_load(
            path.string().c_str(), &width, &height, &sourceChannels, 4);

        if (!guard.pixels) {
            throw std::runtime_error(
                std::format("Failed to load texture ({}): {}",
                    path.string(), stbi_failure_reason()));
        }

        UploadPixels(device, guard.pixels, width, height);
    }

    void Texture::UploadPixels(SDL_GPUDevice *device, const unsigned char *pixels, int width, int height) {
        const Uint32 pixelDataSize =
                static_cast<Uint32>(width) * static_cast<Uint32>(height) * 4;

        SDL_GPUTextureCreateInfo textureCreateInfo{};
        textureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureCreateInfo.format = m_format;
        textureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        textureCreateInfo.width = static_cast<Uint32>(width);
        textureCreateInfo.height = static_cast<Uint32>(height);
        textureCreateInfo.layer_count_or_depth = 1;
        textureCreateInfo.num_levels = 1;
        textureCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

        m_texture = GPUTextureHandle(
            device, SDL_CreateGPUTexture(device, &textureCreateInfo));

        if (!m_texture) {
            throw std::runtime_error(
                std::format("Failed to create GPU texture: {}", SDL_GetError()));
        }

        SDL_GPUTransferBufferCreateInfo transferCreateInfo{};
        transferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferCreateInfo.size = pixelDataSize;

        GPUTransferBufferHandle transferBuffer(
            device, SDL_CreateGPUTransferBuffer(device, &transferCreateInfo));

        if (!transferBuffer) {
            throw std::runtime_error(
                std::format("Failed to create transfer buffer: {}", SDL_GetError()));
        }

        void *mapped = SDL_MapGPUTransferBuffer(device, transferBuffer.Get(), false);

        if (!mapped) {
            throw std::runtime_error(
                std::format("Failed to map transfer buffer: {}", SDL_GetError()));
        }

        std::memcpy(mapped, pixels, pixelDataSize);
        SDL_UnmapGPUTransferBuffer(device, transferBuffer.Get());

        SDL_GPUCommandBuffer *uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

        SDL_GPUTextureTransferInfo source{};
        source.transfer_buffer = transferBuffer.Get();
        source.offset = 0;
        source.pixels_per_row = static_cast<Uint32>(width);
        source.rows_per_layer = static_cast<Uint32>(height);

        SDL_GPUTextureRegion destination{};
        destination.texture = m_texture.Get();
        destination.mip_level = 0;
        destination.layer = 0;
        destination.x = 0;
        destination.y = 0;
        destination.z = 0;
        destination.w = static_cast<Uint32>(width);
        destination.h = static_cast<Uint32>(height);
        destination.d = 1;

        SDL_UploadToGPUTexture(copyPass, &source, &destination, false);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

        // transferBuffer releases itself here (RAII), no manual call needed.
    }
}
