#include <Engine/Scene/Scene.h>

#include <Engine/ECS/Components.h>
#include <Engine/ECS/Systems.h>
#include <Engine/Renderer/Camera.h>
#include <Engine/Renderer/GlmConfig.h>
#include <Engine/Renderer/Light.h>
#include <Engine/Renderer/Model.h>
#include <Engine/Renderer/Sampler.h>

#include <glm/glm.hpp>

#include <cmath>

namespace Engine {
    Scene::Scene(SDL_GPUDevice *device, const Sampler &sampler)
        : m_device(device), m_sampler(&sampler) {
    }

    void Scene::Update(float deltaTime, Input &input, AppMode mode) {
        if (mode == AppMode::Game) {
            const auto cameraView = m_registry.view<Camera, ActiveCamera>();
            m_registry.get<Camera>(*cameraView.begin()).Update(deltaTime, input);
        }

        RotateSystem(m_registry, deltaTime);

        // Slow arc across the sky around the diorama's vertical axis, tilted
        // down toward the ground — a primitive day/night cycle.
        constexpr float kLightRotationSpeed = glm::radians(6.0f); // rad/sec
        m_lightAngle += kLightRotationSpeed * deltaTime;

        const auto lightView = m_registry.view<DirectionalLight>();
        DirectionalLight &light = m_registry.get<DirectionalLight>(*lightView.begin());
        light.Direction = glm::normalize(glm::vec3(
            std::cos(m_lightAngle), -0.6f, std::sin(m_lightAngle)));
    }

    AssetHandle<Model> Scene::LoadModel(const std::filesystem::path &path) {
        return m_assets.Models.Load(
            path,
            [this, path] {
                return std::make_shared<Model>(m_device, path, *m_sampler, m_assets);
            });
    }

    void Scene::ReloadChangedAssets() {
        m_assets.ReloadChanged();
    }
}
