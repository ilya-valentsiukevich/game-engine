//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include "../include/Engine/Application.h"

#include <SDL3/SDL.h>
#include <iostream>

namespace {
    SDL_Window *g_Window = nullptr;
}

Application::Application() = default;

Application::~Application() = default;

bool Application::Initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: "
                << SDL_GetError()
                << std::endl;

        return false;
    }

    g_Window = SDL_CreateWindow(
        "Game Engine",
        1280,
        720,
        0);

    if (!g_Window) {
        std::cerr << "Failed to create window: "
                << SDL_GetError()
                << std::endl;

        SDL_Quit();
        return false;
    }

    m_Running = true;

    return true;
}

void Application::Run() {
    while (m_Running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_Running = false;
            }
        }

        SDL_Delay(1);
    }
}

void Application::Shutdown() {
    SDL_DestroyWindow(g_Window);
    SDL_Quit();
}
