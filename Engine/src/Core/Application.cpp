//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include "../include/Engine/Core/Application.h"

#include <SDL3/SDL.h>

bool Application::Initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO))
        return false;

    if (!m_window.Create("My Engine", 1280, 720))
        return false;

    if (!m_renderer.Initialize(m_window))
        return false;

    return true;
}

void Application::Run() {
    while (m_running) {
        PollEvents();

        // Update();
        // Render();

        SDL_Delay(1);
    }
}

void Application::Shutdown() {
    m_renderer.Shutdown();
    m_window.Destroy();

    SDL_Quit();
}

void Application::PollEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                m_running = false;
                break;

            default:
                break;
        }
    }
}
