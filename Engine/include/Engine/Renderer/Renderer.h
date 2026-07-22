//
// Created by Ilya Valentsiukevich on 22/07/2026.
//
#pragma once

#include "../Window/Window.h"

class Renderer {
public:
    bool Initialize(Window &window);

    void BeginFrame();

    void EndFrame();

    void Shutdown();
};
