#pragma once

namespace Engine {
    // Which "layer" currently owns mouse/keyboard input and what the debug
    // UI shows, the same Game/Editor split most engines expose. Toggled by
    // Escape in Application::PollEvents; Renderer/Camera/DebugUI all read
    // it to decide what to do with input and what to draw, rather than
    // each independently polling ImGui's io.WantCaptureMouse or SDL's
    // relative-mouse-mode flag.
    enum class AppMode {
        Game,  // camera flies (WASD + mouse-look); Entities/Inspector hidden
        Debug, // camera frozen, cursor free; Entities/Inspector visible
    };
}
