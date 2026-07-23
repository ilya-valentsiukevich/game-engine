//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include <Engine/Core/Application.h>
#include <SDL3/SDL.h>

Application::Application()
    : m_window("My Engine", 1280, 720), m_renderer(m_window) {
}

void Application::Run() {
    while (m_running) {
        PollEvents();
        Update();
        Render();

        SDL_Delay(1);
    }
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

void Application::Update() {
}

void Application::Render() {
    if (m_renderer.BeginFrame()) {
        m_renderer.Render();
        m_renderer.EndFrame();
    }
}
