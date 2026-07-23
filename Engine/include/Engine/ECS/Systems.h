#pragma once

#include <Engine/Renderer/GlmConfig.h>

#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace Engine {
    // Advances every (Transform, Spin) entity by one tick: rotates both
    // its position and orientation by the same increment around Spin::Axis.
    void RotateSystem(entt::registry &registry, float deltaTime);

    // Draws every (Transform, MeshRenderer) entity through the caller's
    // already-open render pass: pushes {viewProjection * world, world} as
    // the vertex uniform, then calls Model::Draw().
    void RenderSystem(entt::registry &registry, SDL_GPUCommandBuffer *commandBuffer,
                       SDL_GPURenderPass *renderPass, const glm::mat4 &viewProjection);

    // Draws every (Transform, MeshRenderer) entity's geometry into the
    // shadow map's depth target, from the light's point of view — pushes
    // lightSpaceMatrix * world as the vertex uniform, then calls
    // Model::DrawDepthOnly().
    void ShadowSystem(entt::registry &registry, SDL_GPUCommandBuffer *commandBuffer,
                       SDL_GPURenderPass *renderPass, const glm::mat4 &lightSpaceMatrix);
}
