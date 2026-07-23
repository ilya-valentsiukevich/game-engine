//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#pragma once

#include <Engine/Core/AppMode.h>
#include <Engine/Core/SDLContext.h>
#include <Engine/Core/Input.h>
#include <Engine/Window/Window.h>
#include <Engine/Renderer/Renderer.h>
#include <Engine/Scene/Scene.h>

namespace Engine {
    class Application {
    public:
        Application();

        ~Application() = default;

        Application(const Application &) = delete;
        Application &operator=(const Application &) = delete;

        void Run();

        // Lets Game code populate the scene (camera, lights, models, ...)
        // through Scene's public API before the first Run() tick — see
        // Game/main.cpp.
        Scene &GetScene() { return m_scene; }

    private:
        void PollEvents();

        void Update(float deltaTime);

        void Render();

    private:
        bool m_running = true;

        // Declaration order matters: m_sdl must be constructed before, and
        // destroyed after, m_window and m_renderer, since they depend on SDL
        // being initialized for their entire lifetime. m_scene is declared
        // after m_renderer both because its constructor needs the GPU
        // device/sampler m_renderer owns, and because its AssetHandles must
        // be destroyed before that GPU device is.
        SDLContext m_sdl;
        Window m_window;
        Renderer m_renderer;
        Scene m_scene;

        Input m_input;
        AppMode m_mode = AppMode::Game;
    };
}
