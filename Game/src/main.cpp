#include <Engine/Core/Application.h>
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Transform.h>
#include <Engine/Renderer/Camera.h>
#include <Engine/Renderer/Light.h>
#include <Engine/Renderer/Model.h>
#include <Engine/Scene/Scene.h>

#include <SDL3/SDL.h>

#include <cmath>
#include <exception>
#include <iterator>

namespace {
    // Five KayKit characters arranged in a circle, each slowly orbiting the
    // diorama's center while spinning in place (Spin component) — the demo
    // scene the engine has shown since M6.
    void BuildDiorama(Engine::Scene &scene) {
        entt::registry &registry = scene.Registry();

        const entt::entity cameraEntity = registry.create();
        registry.emplace<Engine::Camera>(cameraEntity);
        registry.emplace<Engine::ActiveCamera>(cameraEntity);
        registry.emplace<Engine::Name>(cameraEntity, "Camera");

        const entt::entity lightEntity = registry.create();
        registry.emplace<Engine::DirectionalLight>(lightEntity);
        registry.emplace<Engine::Name>(lightEntity, "Sun");

        const entt::entity torchOneEntity = registry.create();
        Engine::Transform &torchOneTransform = registry.emplace<Engine::Transform>(torchOneEntity);
        torchOneTransform.Position = glm::vec3(1.5f, 1.2f, 1.5f);
        registry.emplace<Engine::PointLight>(torchOneEntity, glm::vec3(1.0f, 0.55f, 0.2f));
        registry.emplace<Engine::Name>(torchOneEntity, "Torch 1");

        const entt::entity torchTwoEntity = registry.create();
        Engine::Transform &torchTwoTransform = registry.emplace<Engine::Transform>(torchTwoEntity);
        torchTwoTransform.Position = glm::vec3(-1.5f, 1.2f, -1.5f);
        registry.emplace<Engine::PointLight>(torchTwoEntity, glm::vec3(0.2f, 0.4f, 1.0f));
        registry.emplace<Engine::Name>(torchTwoEntity, "Torch 2");

        const entt::entity spotlightEntity = registry.create();
        Engine::Transform &spotlightTransform = registry.emplace<Engine::Transform>(spotlightEntity);
        spotlightTransform.Position = glm::vec3(0.0f, 4.0f, 0.0f);
        registry.emplace<Engine::SpotLight>(spotlightEntity, glm::vec3(0.0f, -1.0f, 0.0f));
        registry.emplace<Engine::Name>(spotlightEntity, "Spotlight");

        struct DioramaCharacter {
            const char *name;
            const char *modelPath;
        };

        constexpr DioramaCharacter kDioramaCharacters[] = {
            {"Knight", "Assets/Models/Knight/Knight.glb"},
            {"Barbarian", "Assets/Models/Barbarian/Barbarian.glb"},
            {"Mage", "Assets/Models/Mage/Mage.glb"},
            {"Ranger", "Assets/Models/Ranger/Ranger.glb"},
            {"Rogue", "Assets/Models/Rogue/Rogue_Hooded.glb"},
        };
        constexpr int kCharacterCount =
                static_cast<int>(std::size(kDioramaCharacters));
        constexpr float kRadius = 3.0f;
        constexpr float kPlatformSpinSpeed = glm::radians(30.0f); // rad/sec

        for (int i = 0; i < kCharacterCount; ++i) {
            const DioramaCharacter &character = kDioramaCharacters[i];

            Engine::AssetHandle<Engine::Model> model = scene.LoadModel(character.modelPath);

            const float angle =
                    glm::radians(360.0f / static_cast<float>(kCharacterCount) * static_cast<float>(i));

            const entt::entity entity = registry.create();

            Engine::Transform &transform = registry.emplace<Engine::Transform>(entity);
            transform.Position = glm::vec3(std::cos(angle) * kRadius, 0.0f, std::sin(angle) * kRadius);
            transform.Rotation = glm::angleAxis(-angle, glm::vec3(0.0f, 1.0f, 0.0f));

            registry.emplace<Engine::MeshRenderer>(entity, std::move(model));
            registry.emplace<Engine::Spin>(entity, glm::vec3(0.0f, 1.0f, 0.0f), kPlatformSpinSpeed);
            registry.emplace<Engine::Name>(entity, character.name);
        }
    }
}

int main() {
    try {
        Engine::Application app;
        BuildDiorama(app.GetScene());
        app.Run();
    } catch (const std::exception &e) {
        SDL_Log("Fatal error: %s", e.what());
        return 1;
    }

    return 0;
}
