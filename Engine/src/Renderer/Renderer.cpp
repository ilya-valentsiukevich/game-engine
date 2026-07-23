//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include <Engine/Renderer/Renderer.h>
#include <Engine/Renderer/GPUDevice.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/Model.h>
#include <Engine/Renderer/Camera.h>
#include <Engine/Renderer/Light.h>
#include <Engine/Renderer/Sampler.h>
#include <Engine/Renderer/DebugUI.h>
#include <Engine/Window/Window.h>
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Systems.h>
#include <Engine/ECS/Transform.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <format>
#include <iterator>

namespace Engine {
    Renderer::Renderer(Window &window)
        : m_window(&window), m_device(std::make_unique<GPUDevice>()) {
        SDL_Log(
            "GPU Driver: %s",
            SDL_GetGPUDeviceDriver(m_device->Get())
        );

        if (!SDL_ClaimWindowForGPUDevice(
            m_device->Get(),
            m_window->GetNativeWindow())) {
            throw std::runtime_error(
                std::format("Failed to claim window: {}", SDL_GetError()));
        }

        const Shader vertexShader(
            m_device->Get(),
            "Assets/Shaders/Compiled/Mesh.vert.msl",
            SDL_GPU_SHADERSTAGE_VERTEX,
            1);

        const Shader fragmentShader(
            m_device->Get(),
            "Assets/Shaders/Compiled/Mesh.frag.msl",
            SDL_GPU_SHADERSTAGE_FRAGMENT,
            1,  // numUniformBuffers
            1); // numSamplers

        const SDL_GPUTextureFormat colorFormat = SDL_GetGPUSwapchainTextureFormat(
            m_device->Get(),
            m_window->GetNativeWindow());

        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSizeInPixels(
            m_window->GetNativeWindow(), &windowWidth, &windowHeight);

        CreateDepthTexture(static_cast<Uint32>(windowWidth), static_cast<Uint32>(windowHeight));

        m_pipeline = std::make_unique<Pipeline>(
            m_device->Get(), colorFormat, kDepthFormat, vertexShader, fragmentShader);

        m_sampler = std::make_unique<Sampler>(
            m_device->Get(),
            SDL_GPU_FILTER_LINEAR,
            SDL_GPU_SAMPLERADDRESSMODE_REPEAT);

        m_debugUI = std::make_unique<DebugUI>(*m_window, m_device->Get(), colorFormat);

        m_cameraEntity = m_registry.create();
        m_registry.emplace<Camera>(m_cameraEntity);
        m_registry.emplace<Name>(m_cameraEntity, "Camera");

        m_lightEntity = m_registry.create();
        m_registry.emplace<DirectionalLight>(m_lightEntity);
        m_registry.emplace<Name>(m_lightEntity, "Sun");

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

            AssetHandle<Model> model = m_assets.Models.Load(
                character.modelPath,
                [this, path = std::filesystem::path(character.modelPath)] {
                    return std::make_shared<Model>(m_device->Get(), path, *m_sampler, m_assets);
                });

            const float angle =
                    glm::radians(360.0f / static_cast<float>(kCharacterCount) * static_cast<float>(i));

            const entt::entity entity = m_registry.create();

            Transform &transform = m_registry.emplace<Transform>(entity);
            transform.Position = glm::vec3(std::cos(angle) * kRadius, 0.0f, std::sin(angle) * kRadius);
            transform.Rotation = glm::angleAxis(-angle, glm::vec3(0.0f, 1.0f, 0.0f));

            m_registry.emplace<MeshRenderer>(entity, std::move(model));
            m_registry.emplace<Spin>(entity, glm::vec3(0.0f, 1.0f, 0.0f), kPlatformSpinSpeed);
            m_registry.emplace<Name>(entity, character.name);
        }
    }

    bool Renderer::BeginFrame() {
        m_commandBuffer = SDL_AcquireGPUCommandBuffer(m_device->Get());

        if (!m_commandBuffer) {
            SDL_Log("Failed to acquire command buffer.");
            return false;
        }

        Uint32 width = 0;
        Uint32 height = 0;

        if (!SDL_AcquireGPUSwapchainTexture(
            m_commandBuffer,
            m_window->GetNativeWindow(),
            &m_swapchainTexture,
            &width,
            &height)) {
            SDL_Log("Failed to acquire swapchain texture.");

            SDL_CancelGPUCommandBuffer(m_commandBuffer);

            m_commandBuffer = nullptr;
            return false;
        }

        if (m_swapchainTexture == nullptr) {
            SDL_SubmitGPUCommandBuffer(m_commandBuffer);

            m_commandBuffer = nullptr;
            return false;
        }

        SDL_GPUColorTargetInfo colorTarget{};
        colorTarget.texture = m_swapchainTexture;

        colorTarget.clear_color = {
            0.0f,
            0.2f,
            1.0f,
            1.0f
        };

        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPUDepthStencilTargetInfo depthTarget{};
        depthTarget.texture = m_depthTexture.Get();
        depthTarget.clear_depth = 1.0f;
        depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

        m_renderPass = SDL_BeginGPURenderPass(
            m_commandBuffer,
            &colorTarget,
            1,
            &depthTarget);

        return true;
    }

    void Renderer::EndFrame() {
        if (!m_commandBuffer)
            return;

        if (m_renderPass) {
            SDL_EndGPURenderPass(m_renderPass);
            m_renderPass = nullptr;
        }

        SDL_SubmitGPUCommandBuffer(m_commandBuffer);

        m_commandBuffer = nullptr;
        m_swapchainTexture = nullptr;
    }

    Renderer::~Renderer() {
        if (!m_device)
            return;

        // m_depthTexture/m_pipeline/m_sampler/m_registry release themselves
        // via their own destructors (declared after m_device, so they run
        // first). m_device itself is destroyed last.
        SDL_ReleaseWindowFromGPUDevice(
            m_device->Get(),
            m_window->GetNativeWindow());
    }

    void Renderer::Update(float deltaTime, Input &input, AppMode mode) {
        if (mode == AppMode::Game)
            m_registry.get<Camera>(m_cameraEntity).Update(deltaTime, input);

        RotateSystem(m_registry, deltaTime);

        // Slow arc across the sky around the diorama's vertical axis, tilted
        // down toward the ground — a primitive day/night cycle.
        constexpr float kLightRotationSpeed = glm::radians(6.0f); // rad/sec
        m_lightAngle += kLightRotationSpeed * deltaTime;

        DirectionalLight &light = m_registry.get<DirectionalLight>(m_lightEntity);
        light.Direction = glm::normalize(glm::vec3(
            std::cos(m_lightAngle), -0.6f, std::sin(m_lightAngle)));
    }

    void Renderer::Render(AppMode mode) {
        if (!m_renderPass)
            return;

        m_debugUI->Draw(m_registry, mode);

        SDL_BindGPUGraphicsPipeline(m_renderPass, m_pipeline->Get());

        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSizeInPixels(m_window->GetNativeWindow(), &windowWidth, &windowHeight);
        const float aspectRatio =
                static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

        const Camera &camera = m_registry.get<Camera>(m_cameraEntity);
        const glm::mat4 view = camera.GetViewMatrix();
        const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
        const glm::mat4 viewProjection = projection * view;

        const DirectionalLight &light = m_registry.get<DirectionalLight>(m_lightEntity);

        struct LightUniformBlock {
            glm::vec4 Direction;
            glm::vec4 Color;
            glm::vec4 ViewPosition;
            glm::vec4 Params; // x: ambient, y: specular, z: shininess, w: unused
        };

        const LightUniformBlock lightUniform{
            glm::vec4(light.Direction, 0.0f),
            glm::vec4(light.Color, 0.0f),
            glm::vec4(camera.GetPosition(), 0.0f),
            glm::vec4(light.AmbientStrength, light.SpecularStrength, light.Shininess, 0.0f),
        };

        SDL_PushGPUFragmentUniformData(m_commandBuffer, 0, &lightUniform, sizeof(lightUniform));

        RenderSystem(m_registry, m_commandBuffer, m_renderPass, viewProjection);

        SDL_EndGPURenderPass(m_renderPass);
        m_renderPass = nullptr;

        m_debugUI->FinalizeDrawData(m_commandBuffer);

        SDL_GPUColorTargetInfo uiColorTarget{};
        uiColorTarget.texture = m_swapchainTexture;
        uiColorTarget.load_op = SDL_GPU_LOADOP_LOAD;
        uiColorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass *uiRenderPass =
                SDL_BeginGPURenderPass(m_commandBuffer, &uiColorTarget, 1, nullptr);

        m_debugUI->RenderDrawData(m_commandBuffer, uiRenderPass);

        SDL_EndGPURenderPass(uiRenderPass);
    }

    void Renderer::ReloadChangedAssets() {
        m_assets.ReloadChanged();
    }

    void Renderer::ProcessDebugUIEvent(const SDL_Event &event) {
        m_debugUI->ProcessEvent(event);
    }

    void Renderer::OnWindowResized() {
        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSizeInPixels(m_window->GetNativeWindow(), &windowWidth, &windowHeight);

        if (windowWidth <= 0 || windowHeight <= 0)
            return;

        CreateDepthTexture(static_cast<Uint32>(windowWidth), static_cast<Uint32>(windowHeight));
    }

    void Renderer::CreateDepthTexture(Uint32 width, Uint32 height) {
        SDL_GPUTextureCreateInfo depthCreateInfo{};
        depthCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
        depthCreateInfo.format = kDepthFormat;
        depthCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        depthCreateInfo.width = width;
        depthCreateInfo.height = height;
        depthCreateInfo.layer_count_or_depth = 1;
        depthCreateInfo.num_levels = 1;
        depthCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

        // Assigning over the existing handle releases the old texture via
        // SDL_ReleaseGPUTexture first — safe even if a previously submitted
        // command buffer is still using it, SDL_gpu defers the actual
        // GPU-side destruction until it's no longer in flight.
        m_depthTexture = GPUTextureHandle(
            m_device->Get(), SDL_CreateGPUTexture(m_device->Get(), &depthCreateInfo));

        if (!m_depthTexture) {
            throw std::runtime_error(
                std::format("Failed to create depth texture: {}", SDL_GetError()));
        }
    }
}
