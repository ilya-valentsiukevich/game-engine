//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include <Engine/Renderer/Renderer.h>
#include <Engine/Renderer/GPUDevice.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/Camera.h>
#include <Engine/Renderer/Light.h>
#include <Engine/Renderer/Sampler.h>
#include <Engine/Renderer/ShadowMap.h>
#include <Engine/Renderer/DebugUI.h>
#include <Engine/Window/Window.h>
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Systems.h>
#include <Engine/ECS/Transform.h>
#include <Engine/Scene/Scene.h>

#include <glm/glm.hpp>

#include <format>

namespace Engine {
    namespace {
        // Hand-tuned bounds for the shadow camera's orthographic frustum,
        // sized for BuildDiorama's ~3-unit-radius circle of characters (see
        // Game/main.cpp) — the same "fit the known scene by eye" approach
        // Camera's own near/far planes already take for the main camera.
        constexpr float kShadowOrthoHalfExtent = 6.0f;
        constexpr float kShadowNearPlane = 0.1f;
        constexpr float kShadowFarPlane = 20.0f;
        constexpr float kShadowDistance = 10.0f;
        constexpr glm::vec3 kShadowTarget{0.0f, 0.0f, 0.0f};

        glm::mat4 ComputeLightSpaceMatrix(const DirectionalLight &light) {
            const glm::vec3 direction = glm::normalize(light.Direction);
            const glm::vec3 eye = kShadowTarget - direction * kShadowDistance;

            // glm::lookAt's up vector must not be parallel to (eye -
            // target). DirectionalLight's own default Direction, straight
            // down, is exactly that degenerate case against the usual
            // world-up — fall back to a different axis whenever direction
            // is nearly vertical.
            glm::vec3 up{0.0f, 1.0f, 0.0f};
            if (glm::abs(glm::dot(direction, up)) > 0.999f) {
                up = glm::vec3(0.0f, 0.0f, 1.0f);
            }

            const glm::mat4 lightView = glm::lookAt(eye, kShadowTarget, up);
            const glm::mat4 lightProjection = glm::ortho(
                -kShadowOrthoHalfExtent, kShadowOrthoHalfExtent,
                -kShadowOrthoHalfExtent, kShadowOrthoHalfExtent,
                kShadowNearPlane, kShadowFarPlane);

            return lightProjection * lightView;
        }
    }

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
            2,  // numUniformBuffers: buffer(0) LightUniformBlock, buffer(1) MaterialUniformBlock
            3); // numSamplers: sampler(0) base color, sampler(1) metallic-roughness, sampler(2) shadow map

        const Shader shadowVertexShader(
            m_device->Get(),
            "Assets/Shaders/Compiled/Shadow.vert.msl",
            SDL_GPU_SHADERSTAGE_VERTEX,
            1); // buffer(0): light-space MVP

        const Shader shadowFragmentShader(
            m_device->Get(),
            "Assets/Shaders/Compiled/Shadow.frag.msl",
            SDL_GPU_SHADERSTAGE_FRAGMENT,
            0,  // depth-only: no uniform buffers
            0); // depth-only: no samplers

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

        m_shadowMap = std::make_unique<ShadowMap>(m_device->Get(), kShadowMapSize);
        m_shadowPipeline = std::make_unique<Pipeline>(
            m_device->Get(), ShadowMap::kFormat, shadowVertexShader, shadowFragmentShader);

        m_debugUI = std::make_unique<DebugUI>(*m_window, m_device->Get(), colorFormat);
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

        // The main color+depth render pass opens inside MainPass, not here
        // — a shadow pass needs to run first, on the same command buffer,
        // in its own render pass, and SDL_gpu only allows one render pass
        // open at a time.
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

        // m_depthTexture/m_pipeline/m_sampler release themselves via their
        // own destructors (declared after m_device, so they run first).
        // m_device itself is destroyed last.
        SDL_ReleaseWindowFromGPUDevice(
            m_device->Get(),
            m_window->GetNativeWindow());
    }

    void Renderer::Render(Scene &scene, AppMode mode) {
        if (!m_commandBuffer || !m_swapchainTexture)
            return;

        entt::registry &registry = scene.Registry();
        const auto lightView = registry.view<DirectionalLight>();
        const DirectionalLight &light = registry.get<DirectionalLight>(*lightView.begin());
        const glm::mat4 lightSpaceMatrix = ComputeLightSpaceMatrix(light);

        ShadowPass(scene, lightSpaceMatrix);
        MainPass(scene, lightSpaceMatrix);
        UIPass(scene, mode);
    }

    void Renderer::ShadowPass(Scene &scene, const glm::mat4 &lightSpaceMatrix) {
        entt::registry &registry = scene.Registry();

        SDL_GPUDepthStencilTargetInfo shadowDepthTarget{};
        shadowDepthTarget.texture = m_shadowMap->GetTexture();
        shadowDepthTarget.clear_depth = 1.0f;
        shadowDepthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        // MainPass samples this texture right after ShadowPass ends — the
        // written depth must survive past this render pass, unlike the main
        // depth texture (STORE_OP_DONT_CARE there, nothing reads it back).
        shadowDepthTarget.store_op = SDL_GPU_STOREOP_STORE;
        shadowDepthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        shadowDepthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

        SDL_GPURenderPass *shadowRenderPass =
                SDL_BeginGPURenderPass(m_commandBuffer, nullptr, 0, &shadowDepthTarget);

        SDL_BindGPUGraphicsPipeline(shadowRenderPass, m_shadowPipeline->Get());

        ShadowSystem(registry, m_commandBuffer, shadowRenderPass, lightSpaceMatrix);

        SDL_EndGPURenderPass(shadowRenderPass);
    }

    void Renderer::MainPass(Scene &scene, const glm::mat4 &lightSpaceMatrix) {
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

        m_renderPass = SDL_BeginGPURenderPass(m_commandBuffer, &colorTarget, 1, &depthTarget);

        entt::registry &registry = scene.Registry();

        SDL_BindGPUGraphicsPipeline(m_renderPass, m_pipeline->Get());

        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSizeInPixels(m_window->GetNativeWindow(), &windowWidth, &windowHeight);
        const float aspectRatio =
                static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

        // Assumes exactly one entity tagged ActiveCamera and one
        // DirectionalLight entity exist — enforced by whoever builds the
        // scene (see Game/main.cpp), not re-validated here every frame.
        const auto cameraView = registry.view<Camera, ActiveCamera>();
        const Camera &camera = registry.get<Camera>(*cameraView.begin());

        const glm::mat4 view = camera.GetViewMatrix();
        const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
        const glm::mat4 viewProjection = projection * view;

        const auto lightView = registry.view<DirectionalLight>();
        const DirectionalLight &light = registry.get<DirectionalLight>(*lightView.begin());

        struct PointLightData {
            glm::vec4 Position;
            glm::vec4 Color;
            glm::vec4 Attenuation;
        };

        struct SpotLightData {
            glm::vec4 Position;
            glm::vec4 Direction;
            glm::vec4 Color;
            glm::vec4 Attenuation;
            glm::vec4 ConeAngles;
        };

        struct LightUniformBlock {
            glm::vec4 SunDirection;
            glm::vec4 SunColor;
            glm::vec4 ViewPosition;
            glm::vec4 SunParams; // x: ambient, y: point light count, z: spot light count, w: unused
            glm::mat4 LightSpaceMatrix;
            PointLightData PointLights[kMaxPointLights];
            SpotLightData SpotLights[kMaxSpotLights];
        };

        LightUniformBlock lightUniform{};
        lightUniform.SunDirection = glm::vec4(light.Direction, 0.0f);
        lightUniform.SunColor = glm::vec4(light.Color, 0.0f);
        lightUniform.ViewPosition = glm::vec4(camera.GetPosition(), 0.0f);
        lightUniform.LightSpaceMatrix = lightSpaceMatrix;

        int pointLightCount = 0;
        for (auto [entity, transform, pointLight] : registry.view<Transform, PointLight>().each()) {
            if (pointLightCount >= kMaxPointLights)
                break; // extra point lights beyond kMaxPointLights are silently ignored this frame.

            lightUniform.PointLights[pointLightCount] = PointLightData{
                glm::vec4(transform.Position, 0.0f),
                glm::vec4(pointLight.Color, 0.0f),
                glm::vec4(pointLight.Constant, pointLight.Linear, pointLight.Quadratic, 0.0f),
            };
            ++pointLightCount;
        }

        int spotLightCount = 0;
        for (auto [entity, transform, spotLight] : registry.view<Transform, SpotLight>().each()) {
            if (spotLightCount >= kMaxSpotLights)
                break;

            lightUniform.SpotLights[spotLightCount] = SpotLightData{
                glm::vec4(transform.Position, 0.0f),
                glm::vec4(spotLight.Direction, 0.0f),
                glm::vec4(spotLight.Color, 0.0f),
                glm::vec4(spotLight.Constant, spotLight.Linear, spotLight.Quadratic, 0.0f),
                glm::vec4(glm::cos(glm::radians(spotLight.InnerConeAngleDegrees)),
                          glm::cos(glm::radians(spotLight.OuterConeAngleDegrees)), 0.0f, 0.0f),
            };
            ++spotLightCount;
        }

        lightUniform.SunParams = glm::vec4(
            light.AmbientStrength, static_cast<float>(pointLightCount),
            static_cast<float>(spotLightCount), 0.0f);

        SDL_PushGPUFragmentUniformData(m_commandBuffer, 0, &lightUniform, sizeof(lightUniform));

        SDL_GPUTextureSamplerBinding shadowBinding{};
        shadowBinding.texture = m_shadowMap->GetTexture();
        shadowBinding.sampler = m_shadowMap->GetSampler();
        SDL_BindGPUFragmentSamplers(m_renderPass, 2, &shadowBinding, 1);

        RenderSystem(registry, m_commandBuffer, m_renderPass, viewProjection);

        SDL_EndGPURenderPass(m_renderPass);
        m_renderPass = nullptr;
    }

    void Renderer::UIPass(Scene &scene, AppMode mode) {
        m_debugUI->Draw(scene.Registry(), mode);
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

    SDL_GPUDevice *Renderer::GetDevice() const {
        return m_device->Get();
    }

    const Sampler &Renderer::GetDefaultSampler() const {
        return *m_sampler;
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
