//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#pragma once

#include "Engine/Window/Window.h"
#include "Engine/Renderer/Renderer.h"

class Application {
public:
    bool Initialize();

    void Run();

    void Shutdown();

private:
    void PollEvents();

private:
    bool m_running = true;
    Window m_window;
    Renderer m_renderer;
};
