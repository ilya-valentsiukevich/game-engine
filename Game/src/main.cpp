#include <Engine/Core/Application.h>
#include <SDL3/SDL.h>

#include <exception>

int main() {
    try {
        Engine::Application app;
        app.Run();
    } catch (const std::exception &e) {
        SDL_Log("Fatal error: %s", e.what());
        return 1;
    }

    return 0;
}
