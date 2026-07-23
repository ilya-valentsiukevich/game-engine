#include <Engine/Renderer/DebugUI.h>
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Transform.h>
#include <Engine/Renderer/Camera.h>
#include <Engine/Renderer/Light.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <imgui_stdlib.h>

#include <glm/gtc/quaternion.hpp>

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

    void DebugUI::Draw(entt::registry &registry, AppMode mode) {
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        DrawOverlay();

        if (mode == AppMode::Debug) {
            DrawEntityList(registry);
            DrawInspector(registry);
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

        if (registry.all_of<MeshRenderer>(m_selectedEntity)) {
            ImGui::BulletText("Has MeshRenderer");
        }

        if (DirectionalLight *light = registry.try_get<DirectionalLight>(m_selectedEntity)) {
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit3("Color", &light->Color.x);
                ImGui::SliderFloat("Ambient Strength", &light->AmbientStrength, 0.0f, 1.0f);
                ImGui::SliderFloat("Specular Strength", &light->SpecularStrength, 0.0f, 1.0f);
                ImGui::SliderFloat("Shininess", &light->Shininess, 1.0f, 128.0f);

                // Not editable: overwritten every Update() tick by
                // Renderer's day-night cycle — an edit here would just be
                // undone on the next frame.
                ImGui::Text("Direction (animated, read-only): %.2f, %.2f, %.2f",
                            light->Direction.x, light->Direction.y, light->Direction.z);
            }
        }

        ImGui::End();
    }
}
