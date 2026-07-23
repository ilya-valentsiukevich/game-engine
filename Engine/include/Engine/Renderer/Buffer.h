//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/GPUResource.h>

#include <cstring>
#include <format>
#include <span>
#include <stdexcept>

// RAII GPU buffer holding a fixed array of T, uploaded once at construction
// via a transfer buffer. Template because the element type (Vertex, index
// type, ...) determines size/layout but not the upload mechanics.
template<typename T>
class Buffer {
public:
    Buffer(SDL_GPUDevice *device, SDL_GPUBufferUsageFlags usage, std::span<const T> data)
        : m_count(static_cast<Uint32>(data.size())) {
        const Uint32 size = static_cast<Uint32>(data.size_bytes());

        SDL_GPUBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.usage = usage;
        bufferCreateInfo.size = size;

        m_buffer = GPUBufferHandle(device, SDL_CreateGPUBuffer(device, &bufferCreateInfo));

        if (!m_buffer) {
            throw std::runtime_error(
                std::format("Failed to create buffer: {}", SDL_GetError()));
        }

        SDL_GPUTransferBufferCreateInfo transferCreateInfo{};
        transferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferCreateInfo.size = size;

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

        std::memcpy(mapped, data.data(), size);
        SDL_UnmapGPUTransferBuffer(device, transferBuffer.Get());

        SDL_GPUCommandBuffer *uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

        SDL_GPUTransferBufferLocation source{};
        source.transfer_buffer = transferBuffer.Get();
        source.offset = 0;

        SDL_GPUBufferRegion destination{};
        destination.buffer = m_buffer.Get();
        destination.offset = 0;
        destination.size = size;

        SDL_UploadToGPUBuffer(copyPass, &source, &destination, false);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

        // transferBuffer releases itself here (RAII), no manual call needed.
    }

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    SDL_GPUBuffer *Get() const {
        return m_buffer.Get();
    }

    Uint32 Count() const {
        return m_count;
    }

private:
    GPUBufferHandle m_buffer;
    Uint32 m_count = 0;
};
