#pragma once

#include <SDL3/SDL.h>

#include <utility>

namespace Engine {
    // Thin wrapper over SDL's input state. Keyboard is polled on demand via
    // SDL_GetKeyboardState (a live array that always reflects "currently
    // held", no bookkeeping needed). Mouse motion has no polled equivalent in
    // SDL — only per-event xrel/yrel while the window is in relative mouse
    // mode — so it's accumulated from events and handed out once consumed.
    class Input {
    public:
        bool IsKeyDown(SDL_Scancode scancode) const;

        // Call from Application::PollEvents for every SDL_EVENT_MOUSE_MOTION.
        void OnMouseMotion(float xrel, float yrel);

        // Returns the mouse delta accumulated since the last call, then resets it.
        std::pair<float, float> ConsumeMouseDelta();

    private:
        float m_mouseDeltaX = 0.0f;
        float m_mouseDeltaY = 0.0f;
    };
}
