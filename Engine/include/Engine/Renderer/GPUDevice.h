//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <SDL3/SDL_gpu.h>

class GPUDevice {
public:
    GPUDevice();
    ~GPUDevice();

    GPUDevice(const GPUDevice &) = delete;
    GPUDevice &operator=(const GPUDevice &) = delete;

    SDL_GPUDevice *Get() const {
        return m_device;
    }

private:
    SDL_GPUDevice *m_device{};
};
