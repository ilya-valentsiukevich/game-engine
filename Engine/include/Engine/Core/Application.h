//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#pragma once

#include "Engine/Core/SDLContext.h"
#include "Engine/Window/Window.h"
#include "Engine/Renderer/Renderer.h"

class Application {
public:
    Application();

    ~Application() = default;

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    void Run();

private:
    void PollEvents();

    void Update();

    void Render();

private:
    bool m_running = true;

    // Declaration order matters: m_sdl must be constructed before, and
    // destroyed after, m_window and m_renderer, since they depend on SDL
    // being initialized for their entire lifetime.
    SDLContext m_sdl;
    Window m_window;
    Renderer m_renderer;
};
