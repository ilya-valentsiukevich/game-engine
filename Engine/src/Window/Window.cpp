//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include "../include/Engine/Window/Window.h"

bool Window::Create(const char *title,
                    int width,
                    int height) {
    m_window = SDL_CreateWindow(
        title,
        width,
        height,
        SDL_WINDOW_RESIZABLE);

    return m_window != nullptr;
}

void Window::Destroy() {
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
}

SDL_Window *Window::GetNativeWindow() const {
    return m_window;
}
