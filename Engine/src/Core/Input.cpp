#include <Engine/Core/Input.h>

namespace Engine {
    bool Input::IsKeyDown(SDL_Scancode scancode) const {
        const bool *keyboardState = SDL_GetKeyboardState(nullptr);
        return keyboardState[scancode];
    }

    void Input::OnMouseMotion(float xrel, float yrel) {
        m_mouseDeltaX += xrel;
        m_mouseDeltaY += yrel;
    }

    std::pair<float, float> Input::ConsumeMouseDelta() {
        const std::pair delta{m_mouseDeltaX, m_mouseDeltaY};
        m_mouseDeltaX = 0.0f;
        m_mouseDeltaY = 0.0f;
        return delta;
    }
}
