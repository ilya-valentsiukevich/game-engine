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
    Texture::Texture(SDL_GPUDevice *device, const std::filesystem::path &path) {
        int width = 0;
        int height = 0;
        int sourceChannels = 0;

        // Force 4 channels (RGBA) regardless of the source file's actual
        // channel count, since SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM below
        // expects a tightly packed 4-byte-per-pixel layout.
        stbi_uc *pixels = stbi_load(
            path.string().c_str(), &width, &height, &sourceChannels, 4);

        if (!pixels) {
            throw std::runtime_error(
                std::format("Failed to load texture ({}): {}",
                    path.string(), stbi_failure_reason()));
        }

        const Uint32 pixelDataSize =
                static_cast<Uint32>(width) * static_cast<Uint32>(height) * 4;

        SDL_GPUTextureCreateInfo textureCreateInfo{};
        textureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureCreateInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        textureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        textureCreateInfo.width = static_cast<Uint32>(width);
        textureCreateInfo.height = static_cast<Uint32>(height);
        textureCreateInfo.layer_count_or_depth = 1;
        textureCreateInfo.num_levels = 1;
        textureCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

        m_texture = GPUTextureHandle(
            device, SDL_CreateGPUTexture(device, &textureCreateInfo));

        if (!m_texture) {
            stbi_image_free(pixels);
            throw std::runtime_error(
                std::format("Failed to create GPU texture: {}", SDL_GetError()));
        }

        SDL_GPUTransferBufferCreateInfo transferCreateInfo{};
        transferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferCreateInfo.size = pixelDataSize;

        GPUTransferBufferHandle transferBuffer(
            device, SDL_CreateGPUTransferBuffer(device, &transferCreateInfo));

        if (!transferBuffer) {
            stbi_image_free(pixels);
            throw std::runtime_error(
                std::format("Failed to create transfer buffer: {}", SDL_GetError()));
        }

        void *mapped = SDL_MapGPUTransferBuffer(device, transferBuffer.Get(), false);

        if (!mapped) {
            stbi_image_free(pixels);
            throw std::runtime_error(
                std::format("Failed to map transfer buffer: {}", SDL_GetError()));
        }

        std::memcpy(mapped, pixels, pixelDataSize);
        SDL_UnmapGPUTransferBuffer(device, transferBuffer.Get());
        stbi_image_free(pixels);

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
