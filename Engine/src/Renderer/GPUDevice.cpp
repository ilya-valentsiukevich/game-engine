//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/GPUDevice.h>

#include <format>
#include <stdexcept>

namespace Engine {
    GPUDevice::GPUDevice() {
        m_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL,
                                       true,
                                       nullptr);

        if (!m_device) {
            throw std::runtime_error(
                std::format("Failed to create GPU device: {}", SDL_GetError()));
        }
    }

    GPUDevice::~GPUDevice() {
        SDL_DestroyGPUDevice(m_device);
    }
}
