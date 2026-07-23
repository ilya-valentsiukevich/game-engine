add_library(Engine STATIC
        src/Core/Application.cpp
        src/Core/SDLContext.cpp
        src/Window/Window.cpp
        src/Renderer/Renderer.cpp
        src/Renderer/GPUDevice.cpp
        src/Renderer/Shader.cpp
        src/Renderer/Pipeline.cpp
        src/Assets/ShaderLoader/ShaderLoader.cpp
)

target_include_directories(Engine
        PUBLIC
        include
)

find_package(SDL3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

target_link_libraries(Engine
        PUBLIC
        SDL3::SDL3
        fmt::fmt
        glm::glm
)

target_compile_features(Engine PUBLIC cxx_std_23)