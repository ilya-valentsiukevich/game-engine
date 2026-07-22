//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include <SDL3/SDL_video.h>

class Window {
public:
    bool Create(
        const char *title,
        int width,
        int height);

    void Destroy();

    SDL_Window *GetNativeWindow() const;

private:
    SDL_Window *m_window = nullptr;
};
