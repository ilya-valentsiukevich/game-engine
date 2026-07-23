//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include <SDL3/SDL_video.h>

class Window {
public:
    Window(const char *title, int width, int height);

    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    SDL_Window *GetNativeWindow() const;

private:
    SDL_Window *m_window = nullptr;
};
