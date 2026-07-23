#include <Engine/ECS/Systems.h>
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Transform.h>
#include <Engine/Renderer/Model.h>

#include <glm/gtc/quaternion.hpp>

namespace Engine {
    void RotateSystem(entt::registry &registry, float deltaTime) {
        const auto view = registry.view<Transform, Spin>();

        for (auto [entity, transform, spin] : view.each()) {
            const glm::quat delta = glm::angleAxis(spin.AngularSpeed * deltaTime, spin.Axis);

            transform.Position = delta * transform.Position;
            transform.Rotation = delta * transform.Rotation;
        }
    }

    void RenderSystem(entt::registry &registry, SDL_GPUCommandBuffer *commandBuffer,
                       SDL_GPURenderPass *renderPass, const glm::mat4 &viewProjection) {
        struct VertexUniformBlock {
            glm::mat4 MVP;
            glm::mat4 Model;
            glm::mat4 NormalMatrix;
        };

        const auto view = registry.view<Transform, MeshRenderer>();

        for (auto [entity, transform, meshRenderer] : view.each()) {
            const glm::mat4 world = transform.ToMatrix();

            // Model alone only transforms normals correctly under uniform
            // scale — DebugUI's inspector lets Scale go non-uniform (see
            // DrawInspector's Scale DragFloat3), so the general case needs
            // the inverse-transpose instead. Sent as a float4x4 (not
            // float3x3) purely to match MSL's constant-buffer layout for
            // Model/MVP above: a float3x3's per-column alignment there
            // doesn't line up with glm::mat3's tightly packed layout.
            const glm::mat4 normalMatrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(world))));

            const VertexUniformBlock vertexUniform{viewProjection * world, world, normalMatrix};

            SDL_PushGPUVertexUniformData(commandBuffer, 0, &vertexUniform, sizeof(vertexUniform));
            meshRenderer.Model->Draw(renderPass);
        }
    }
}
