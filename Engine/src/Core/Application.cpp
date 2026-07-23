//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include <Engine/Core/Application.h>
#include <SDL3/SDL.h>

#include <algorithm>

namespace Engine {
    Application::Application()
        : m_window("My Engine", 1280, 720), m_renderer(m_window) {
        SDL_SetWindowRelativeMouseMode(m_window.GetNativeWindow(), true);
    }

    void Application::Run() {
        Uint64 previousCounter = SDL_GetPerformanceCounter();
        const Uint64 frequency = SDL_GetPerformanceFrequency();

        constexpr float kFixedDeltaTime = 1.0f / 60.0f;
        float accumulator = 0.0f;

        while (m_running) {
            const Uint64 currentCounter = SDL_GetPerformanceCounter();
            float frameTime =
                    static_cast<float>(currentCounter - previousCounter) /
                    static_cast<float>(frequency);
            previousCounter = currentCounter;

            // Guards against the "spiral of death" after a long stall (e.g.
            // the window was minimized) by not trying to catch up more than
            // a quarter second of simulation in one go.
            frameTime = std::min(frameTime, 0.25f);

            PollEvents();

            accumulator += frameTime;
            while (accumulator >= kFixedDeltaTime) {
                Update(kFixedDeltaTime);
                accumulator -= kFixedDeltaTime;
            }

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

    void Application::Update(float deltaTime) {
        m_renderer.Update(deltaTime);
    }

    void Application::Render() {
        if (m_renderer.BeginFrame()) {
            m_renderer.Render();
            m_renderer.EndFrame();
        }
    }
}
