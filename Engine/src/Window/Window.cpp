//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include <Engine/Window/Window.h>

#include <SDL3/SDL_error.h>
#include <format>
#include <stdexcept>

namespace Engine {
    Window::Window(const char *title, int width, int height) {
        m_window = SDL_CreateWindow(
            title,
            width,
            height,
            SDL_WINDOW_RESIZABLE);

        if (!m_window) {
            throw std::runtime_error(
                std::format("Failed to create window: {}", SDL_GetError()));
        }
    }

    Window::~Window() {
        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }
    }

    SDL_Window *Window::GetNativeWindow() const {
        return m_window;
    }
}
