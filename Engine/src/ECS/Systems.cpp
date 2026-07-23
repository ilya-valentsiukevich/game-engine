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
        };

        const auto view = registry.view<Transform, MeshRenderer>();

        for (auto [entity, transform, meshRenderer] : view.each()) {
            const glm::mat4 world = transform.ToMatrix();
            const VertexUniformBlock vertexUniform{viewProjection * world, world};

            SDL_PushGPUVertexUniformData(commandBuffer, 0, &vertexUniform, sizeof(vertexUniform));
            meshRenderer.Model->Draw(renderPass);
        }
    }
}
