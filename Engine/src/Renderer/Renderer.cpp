//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include <Engine/Renderer/Renderer.h>
#include <Engine/Renderer/GPUDevice.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/Model.h>
#include <Engine/Renderer/Camera.h>
#include <Engine/Renderer/Sampler.h>
#include <Engine/Window/Window.h>

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

        constexpr SDL_GPUTextureFormat depthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSizeInPixels(
            m_window->GetNativeWindow(), &windowWidth, &windowHeight);

        SDL_GPUTextureCreateInfo depthCreateInfo{};
        depthCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
        depthCreateInfo.format = depthFormat;
        depthCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        depthCreateInfo.width = static_cast<Uint32>(windowWidth);
        depthCreateInfo.height = static_cast<Uint32>(windowHeight);
        depthCreateInfo.layer_count_or_depth = 1;
        depthCreateInfo.num_levels = 1;
        depthCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

        m_depthTexture = GPUTextureHandle(
            m_device->Get(), SDL_CreateGPUTexture(m_device->Get(), &depthCreateInfo));

        if (!m_depthTexture) {
            throw std::runtime_error(
                std::format("Failed to create depth texture: {}", SDL_GetError()));
        }

        m_pipeline = std::make_unique<Pipeline>(
            m_device->Get(), colorFormat, depthFormat, vertexShader, fragmentShader);

        m_sampler = std::make_unique<Sampler>(
            m_device->Get(),
            SDL_GPU_FILTER_LINEAR,
            SDL_GPU_SAMPLERADDRESSMODE_REPEAT);

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

        // Diorama: a "Platform" node that one instance of each character is
        // parented to. Rotating just the platform's local transform in
        // Update() rotates all of them together.
        SceneNode &platform =
                m_scene.GetRoot().AddChild(std::make_unique<SceneNode>("Platform"));
        m_platformNode = &platform;

        constexpr float kRadius = 3.0f;

        m_models.reserve(kCharacterCount);

        for (int i = 0; i < kCharacterCount; ++i) {
            const DioramaCharacter &character = kDioramaCharacters[i];

            AssetHandle<Model> model = m_assets.Models.Load(
                character.modelPath,
                [this, path = std::filesystem::path(character.modelPath)] {
                    return std::make_shared<Model>(m_device->Get(), path, *m_sampler, m_assets);
                });

            m_models.push_back(model);

            const float angle =
                    glm::radians(360.0f / static_cast<float>(kCharacterCount) * static_cast<float>(i));

            auto node = std::make_unique<SceneNode>(character.name);
            node->GetLocalTransform().Position =
                    glm::vec3(std::cos(angle) * kRadius, 0.0f, std::sin(angle) * kRadius);
            node->GetLocalTransform().Rotation =
                    glm::angleAxis(-angle, glm::vec3(0.0f, 1.0f, 0.0f));
            node->AttachedModel = model.get();

            platform.AddChild(std::move(node));
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

        // m_depthTexture/m_pipeline/m_sampler/m_models release themselves via
        // their own destructors (declared after m_device, so they run first).
        // m_device itself is destroyed last.
        SDL_ReleaseWindowFromGPUDevice(
            m_device->Get(),
            m_window->GetNativeWindow());
    }

    void Renderer::Update(float deltaTime) {
        constexpr float kRotationSpeed = glm::radians(30.0f); // rad/sec
        m_platformRotationAngle += kRotationSpeed * deltaTime;

        if (m_platformNode) {
            m_platformNode->GetLocalTransform().Rotation =
                    glm::angleAxis(m_platformRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // Slow arc across the sky around the diorama's vertical axis, tilted
        // down toward the ground — a primitive day/night cycle.
        constexpr float kLightRotationSpeed = glm::radians(6.0f); // rad/sec
        m_lightAngle += kLightRotationSpeed * deltaTime;

        m_light.Direction = glm::normalize(glm::vec3(
            std::cos(m_lightAngle), -0.6f, std::sin(m_lightAngle)));

        m_scene.Update();
    }

    void Renderer::Render(const Camera &camera) {
        if (!m_renderPass)
            return;

        SDL_BindGPUGraphicsPipeline(m_renderPass, m_pipeline->Get());

        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSizeInPixels(m_window->GetNativeWindow(), &windowWidth, &windowHeight);
        const float aspectRatio =
                static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

        const glm::mat4 view = camera.GetViewMatrix();
        const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
        const glm::mat4 viewProjection = projection * view;

        struct LightUniformBlock {
            glm::vec4 Direction;
            glm::vec4 Color;
            glm::vec4 ViewPosition;
            glm::vec4 Params; // x: ambient, y: specular, z: shininess, w: unused
        };

        const LightUniformBlock lightUniform{
            glm::vec4(m_light.Direction, 0.0f),
            glm::vec4(m_light.Color, 0.0f),
            glm::vec4(camera.GetPosition(), 0.0f),
            glm::vec4(m_light.AmbientStrength, m_light.SpecularStrength, m_light.Shininess, 0.0f),
        };

        SDL_PushGPUFragmentUniformData(m_commandBuffer, 0, &lightUniform, sizeof(lightUniform));

        DrawNode(m_scene.GetRoot(), viewProjection);
    }

    void Renderer::ReloadChangedAssets() {
        m_assets.ReloadChanged();
    }

    void Renderer::DrawNode(const SceneNode &node, const glm::mat4 &viewProjection) {
        if (node.AttachedModel) {
            struct VertexUniformBlock {
                glm::mat4 MVP;
                glm::mat4 Model;
            };

            const glm::mat4 &model = node.GetWorldMatrix();
            const VertexUniformBlock vertexUniform{viewProjection * model, model};

            SDL_PushGPUVertexUniformData(m_commandBuffer, 0, &vertexUniform, sizeof(vertexUniform));
            node.AttachedModel->Draw(m_renderPass);
        }

        for (const std::unique_ptr<SceneNode> &child : node.GetChildren()) {
            DrawNode(*child, viewProjection);
        }
    }
}
