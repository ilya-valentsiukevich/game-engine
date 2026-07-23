//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <SDL3/SDL_gpu.h>

// RAII handle for SDL GPU objects released via Release(device, handle).
// Owns at most one handle at a time; releases it on destruction, Reset(), or move-assignment.
template<typename T, void (*Release)(SDL_GPUDevice *, T *)>
class GPUResource {
public:
    GPUResource() = default;

    GPUResource(SDL_GPUDevice *device, T *handle)
        : m_device(device), m_handle(handle) {
    }

    GPUResource(const GPUResource &) = delete;
    GPUResource &operator=(const GPUResource &) = delete;

    GPUResource(GPUResource &&other) noexcept
        : m_device(other.m_device), m_handle(other.m_handle) {
        other.m_device = nullptr;
        other.m_handle = nullptr;
    }

    GPUResource &operator=(GPUResource &&other) noexcept {
        if (this != &other) {
            Reset();
            m_device = other.m_device;
            m_handle = other.m_handle;
            other.m_device = nullptr;
            other.m_handle = nullptr;
        }
        return *this;
    }

    ~GPUResource() {
        Reset();
    }

    void Reset(T *handle = nullptr) {
        if (m_handle) {
            Release(m_device, m_handle);
        }
        m_handle = handle;
    }

    T *Get() const {
        return m_handle;
    }

    explicit operator bool() const {
        return m_handle != nullptr;
    }

private:
    SDL_GPUDevice *m_device = nullptr;
    T *m_handle = nullptr;
};

using GPUShaderHandle = GPUResource<SDL_GPUShader, SDL_ReleaseGPUShader>;
using GPUPipelineHandle = GPUResource<SDL_GPUGraphicsPipeline, SDL_ReleaseGPUGraphicsPipeline>;
using GPUBufferHandle = GPUResource<SDL_GPUBuffer, SDL_ReleaseGPUBuffer>;
using GPUTransferBufferHandle = GPUResource<SDL_GPUTransferBuffer, SDL_ReleaseGPUTransferBuffer>;
