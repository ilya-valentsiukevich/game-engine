//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

// RAII guard for SDL_Init/SDL_Quit. Must be the first member of any class
// that owns SDL resources (Window, Renderer, ...) so it is constructed
// before them and destroyed after them.
class SDLContext {
public:
    SDLContext();
    ~SDLContext();

    SDLContext(const SDLContext &) = delete;
    SDLContext &operator=(const SDLContext &) = delete;
};
