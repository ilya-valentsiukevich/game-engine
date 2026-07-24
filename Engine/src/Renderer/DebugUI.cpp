#include <Engine/Renderer/DebugUI.h>
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Transform.h>
#include <Engine/Renderer/Camera.h>
#include <Engine/Renderer/Light.h>
#include <Engine/Renderer/Model.h>
#include <Engine/Renderer/Material.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <imgui_stdlib.h>

#include <glm/gtc/quaternion.hpp>

#include <cstddef>

namespace Engine {
    DebugUI::DebugUI(Window &window, SDL_GPUDevice *device, SDL_GPUTextureFormat colorFormat) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForSDLGPU(window.GetNativeWindow());

        ImGui_ImplSDLGPU3_InitInfo initInfo{};
        initInfo.Device = device;
        initInfo.ColorTargetFormat = colorFormat;
        ImGui_ImplSDLGPU3_Init(&initInfo);
    }

    DebugUI::~DebugUI() {
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    void DebugUI::ProcessEvent(const SDL_Event &event) {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }

    void DebugUI::Draw(entt::registry &registry, AppMode mode, IBLSettings &iblSettings,
                        PostProcessSettings &postProcessSettings) {
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        DrawOverlay();

        if (mode == AppMode::Debug) {
            DrawEntityList(registry);
            DrawInspector(registry);
            DrawIBLSettings(iblSettings);
            DrawPostProcessSettings(postProcessSettings);
        }
    }

    void DebugUI::FinalizeDrawData(SDL_GPUCommandBuffer *commandBuffer) {
        ImGui::Render();
        ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), commandBuffer);
    }

    void DebugUI::RenderDrawData(SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass) {
        ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderPass);
    }

    void DebugUI::DrawOverlay() {
        constexpr float kPadding = 10.0f;
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        const ImVec2 position(viewport->WorkPos.x + kPadding, viewport->WorkPos.y + kPadding);

        ImGui::SetNextWindowPos(position, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.5f);

        constexpr ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("Overlay", nullptr, flags)) {
            const ImGuiIO &io = ImGui::GetIO();
            ImGui::Text("%.1f FPS (%.3f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
        }
        ImGui::End();
    }

    void DebugUI::DrawEntityList(entt::registry &registry) {
        ImGui::Begin("Entities");

        for (auto [entity, name] : registry.view<Name>().each()) {
            // "##<id>" is an ImGui hidden-id suffix: Selectable() identifies
            // widgets by label text, so two entities sharing a display name
            // would otherwise collide and act as one widget.
            const std::string label = name.Value + "##" + std::to_string(entt::to_integral(entity));

            if (ImGui::Selectable(label.c_str(), entity == m_selectedEntity))
                m_selectedEntity = entity;
        }

        ImGui::End();
    }

    void DebugUI::DrawInspector(entt::registry &registry) {
        ImGui::Begin("Inspector");

        if (m_selectedEntity == entt::null || !registry.valid(m_selectedEntity)) {
            ImGui::TextDisabled("No entity selected.");
            ImGui::End();
            return;
        }

        if (Name *name = registry.try_get<Name>(m_selectedEntity)) {
            ImGui::InputText("Name", &name->Value);
        }

        if (Transform *transform = registry.try_get<Transform>(m_selectedEntity)) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Position", &transform->Position.x, 0.05f);
                ImGui::DragFloat3("Scale", &transform->Scale.x, 0.05f, 0.01f, 10.0f);

                // Not editable: dragging the four raw quaternion floats
                // independently almost immediately breaks the unit-length
                // invariant every other Transform consumer assumes. Shown
                // as Euler degrees purely for readability.
                const glm::vec3 euler = glm::degrees(glm::eulerAngles(transform->Rotation));
                ImGui::Text("Rotation (read-only): %.1f, %.1f, %.1f", euler.x, euler.y, euler.z);
            }
        }

        if (Spin *spin = registry.try_get<Spin>(m_selectedEntity)) {
            if (ImGui::CollapsingHeader("Spin", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat("Angular Speed", &spin->AngularSpeed, 0.01f);
                ImGui::Text("Axis (read-only): %.2f, %.2f, %.2f", spin->Axis.x, spin->Axis.y, spin->Axis.z);
            }
        }

        if (MeshRenderer *meshRenderer = registry.try_get<MeshRenderer>(m_selectedEntity)) {
            if (meshRenderer->Model &&
                ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
                Model &model = *meshRenderer->Model;

                for (std::size_t i = 0; i < model.GetPartCount(); ++i) {
                    Material &material = model.GetMaterial(i);

                    // "##<index>" disambiguates widget IDs between parts —
                    // same reason DrawEntityList suffixes its own labels.
                    ImGui::PushID(static_cast<int>(i));
                    ImGui::Text("Part %zu", i);

                    glm::vec4 baseColorFactor = material.GetBaseColorFactor();
                    if (ImGui::ColorEdit4("Base Color Factor", &baseColorFactor.x))
                        material.SetBaseColorFactor(baseColorFactor);

                    float metallicFactor = material.GetMetallicFactor();
                    if (ImGui::SliderFloat("Metallic Factor", &metallicFactor, 0.0f, 1.0f))
                        material.SetMetallicFactor(metallicFactor);

                    float roughnessFactor = material.GetRoughnessFactor();
                    if (ImGui::SliderFloat("Roughness Factor", &roughnessFactor, 0.0f, 1.0f))
                        material.SetRoughnessFactor(roughnessFactor);

                    ImGui::PopID();
                }
            }
        }

        if (DirectionalLight *light = registry.try_get<DirectionalLight>(m_selectedEntity)) {
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit3("Color", &light->Color.x);
                ImGui::SliderFloat("Ambient Strength", &light->AmbientStrength, 0.0f, 1.0f);

                // Not editable: overwritten every Update() tick by
                // Renderer's day-night cycle — an edit here would just be
                // undone on the next frame.
                ImGui::Text("Direction (animated, read-only): %.2f, %.2f, %.2f",
                            light->Direction.x, light->Direction.y, light->Direction.z);
            }
        }

        if (PointLight *pointLight = registry.try_get<PointLight>(m_selectedEntity)) {
            if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit3("Color", &pointLight->Color.x);
                ImGui::DragFloat("Constant", &pointLight->Constant, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Linear", &pointLight->Linear, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("Quadratic", &pointLight->Quadratic, 0.0001f, 0.0f, 1.0f);
            }
        }

        if (SpotLight *spotLight = registry.try_get<SpotLight>(m_selectedEntity)) {
            if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit3("Color", &spotLight->Color.x);
                ImGui::DragFloat3("Direction", &spotLight->Direction.x, 0.01f, -1.0f, 1.0f);
                ImGui::DragFloat("Constant", &spotLight->Constant, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Linear", &spotLight->Linear, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("Quadratic", &spotLight->Quadratic, 0.0001f, 0.0f, 1.0f);
                ImGui::SliderFloat("Inner Cone Angle", &spotLight->InnerConeAngleDegrees, 1.0f, 89.0f);
                ImGui::SliderFloat("Outer Cone Angle", &spotLight->OuterConeAngleDegrees, 1.0f, 89.0f);
            }
        }

        ImGui::End();
    }

    void DebugUI::DrawIBLSettings(IBLSettings &iblSettings) {
        ImGui::Begin("IBL");

        ImGui::Checkbox("Enabled", &iblSettings.Enabled);

        // Tuning knobs only matter once IBL is actually on — dimming them
        // when it's off is a hint, not a hard requirement, since Renderer
        // still reads iblSettings every frame regardless.
        ImGui::BeginDisabled(!iblSettings.Enabled);
        ImGui::SliderFloat("Min Ambient Roughness", &iblSettings.MinAmbientRoughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Max Reflection LOD", &iblSettings.MaxReflectionLod, 0.0f, 8.0f);
        ImGui::SliderFloat("Ambient Intensity", &iblSettings.AmbientIntensity, 0.0f, 3.0f);
        ImGui::EndDisabled();

        ImGui::End();
    }

    void DebugUI::DrawPostProcessSettings(PostProcessSettings &settings) {
        ImGui::Begin("Post-Processing");

        ImGui::SliderFloat("Exposure", &settings.Exposure, 0.0f, 4.0f);

        ImGui::Checkbox("Bloom Enabled", &settings.BloomEnabled);

        ImGui::BeginDisabled(!settings.BloomEnabled);
        ImGui::SliderFloat("Bloom Threshold", &settings.BloomThreshold, 0.0f, 5.0f);
        ImGui::SliderFloat("Bloom Intensity", &settings.BloomIntensity, 0.0f, 2.0f);
        ImGui::EndDisabled();

        ImGui::End();
    }
}
