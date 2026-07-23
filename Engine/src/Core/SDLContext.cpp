//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Core/SDLContext.h>

#include <SDL3/SDL.h>
#include <format>
#include <stdexcept>

SDLContext::SDLContext() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(
            std::format("Failed to init SDL: {}", SDL_GetError()));
    }
}

SDLContext::~SDLContext() {
    SDL_Quit();
}
