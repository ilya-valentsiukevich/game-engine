add_library(Engine STATIC
        src/Core/Application.cpp
        src/Core/SDLContext.cpp
        src/Core/Input.cpp
        src/Window/Window.cpp
        src/Renderer/Renderer.cpp
        src/Renderer/GPUDevice.cpp
        src/Renderer/Shader.cpp
        src/Renderer/Pipeline.cpp
        src/Renderer/Mesh.cpp
        src/Renderer/Camera.cpp
        src/Renderer/Texture.cpp
        src/Renderer/Sampler.cpp
        src/Renderer/Material.cpp
        src/Renderer/Model.cpp
        src/Scene/Transform.cpp
        src/Scene/SceneNode.cpp
        src/Assets/ShaderLoader/ShaderLoader.cpp
        src/Assets/ModelLoader/GltfLoader.cpp
)

target_include_directories(Engine
        PUBLIC
        include
)

find_package(SDL3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)

find_path(CGLTF_INCLUDE_DIR NAMES cgltf.h)

target_include_directories(Engine
        PRIVATE
        ${Stb_INCLUDE_DIR}
        ${CGLTF_INCLUDE_DIR}
)

target_link_libraries(Engine
        PUBLIC
        SDL3::SDL3
        fmt::fmt
        glm::glm
)

target_compile_features(Engine PUBLIC cxx_std_23)