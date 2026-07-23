//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Vertex.h>

#include <array>
#include <cstddef>
#include <format>
#include <stdexcept>

namespace Engine {
    namespace {
        struct VertexInputState {
            SDL_GPUVertexBufferDescription bufferDescription{};
            std::array<SDL_GPUVertexAttribute, 3> attributes{};
        };

        VertexInputState MakeVertexInputState() {
            VertexInputState state;
            state.bufferDescription.slot = 0;
            state.bufferDescription.pitch = sizeof(Vertex);
            state.bufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            state.bufferDescription.instance_step_rate = 0;

            state.attributes[0].location = 0;
            state.attributes[0].buffer_slot = 0;
            state.attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            state.attributes[0].offset = offsetof(Vertex, Position);

            state.attributes[1].location = 1;
            state.attributes[1].buffer_slot = 0;
            state.attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            state.attributes[1].offset = offsetof(Vertex, Normal);

            state.attributes[2].location = 2;
            state.attributes[2].buffer_slot = 0;
            state.attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            state.attributes[2].offset = offsetof(Vertex, TexCoord);

            return state;
        }
    }

    Pipeline::Pipeline(SDL_GPUDevice *device,
                        SDL_GPUTextureFormat colorFormat,
                        SDL_GPUTextureFormat depthFormat,
                        const Shader &vertexShader,
                        const Shader &fragmentShader) {
        VertexInputState vertexInputState = MakeVertexInputState();

        SDL_GPUColorTargetDescription colorTargetDescription{};
        colorTargetDescription.format = colorFormat;

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.vertex_shader = vertexShader.Get();
        pipelineCreateInfo.fragment_shader = fragmentShader.Get();
        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions =
                &vertexInputState.bufferDescription;
        pipelineCreateInfo.vertex_input_state.num_vertex_buffers = 1;
        pipelineCreateInfo.vertex_input_state.vertex_attributes = vertexInputState.attributes.data();
        pipelineCreateInfo.vertex_input_state.num_vertex_attributes =
                static_cast<Uint32>(vertexInputState.attributes.size());

        pipelineCreateInfo.target_info.color_target_descriptions =
                &colorTargetDescription;
        pipelineCreateInfo.target_info.num_color_targets = 1;
        pipelineCreateInfo.target_info.depth_stencil_format = depthFormat;
        pipelineCreateInfo.target_info.has_depth_stencil_target = true;

        pipelineCreateInfo.depth_stencil_state.enable_depth_test = true;
        pipelineCreateInfo.depth_stencil_state.enable_depth_write = true;
        pipelineCreateInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

        // glTF vertices are wound CCW when viewed from the front, in its
        // own right-handed convention (the spec's own front-face rule).
        // GLM_FORCE_LEFT_HANDED (see GlmConfig.h) makes the view matrix a
        // genuinely left-handed frame — lookAt's (right, up, forward) obey
        // right = cross(up, forward) instead of cross(forward, up) — which
        // flips the parity a same-vertex-order right-handed pipeline would
        // produce. The projection step doesn't introduce a further flip
        // (X/Y keep their sign into clip space), so a front-facing,
        // CCW-authored triangle ends up CLOCKWISE on screen here. If models
        // vanish or render inside-out after this change, that reasoning is
        // wrong for this asset set — flip this one enum to COUNTER_CLOCKWISE.
        pipelineCreateInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
        pipelineCreateInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;

        SDL_GPUGraphicsPipeline *pipeline =
                SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);

        if (!pipeline) {
            throw std::runtime_error(
                std::format("Failed to create pipeline: {}", SDL_GetError()));
        }

        m_pipeline = GPUPipelineHandle(device, pipeline);
    }

    Pipeline::Pipeline(SDL_GPUDevice *device,
                        SDL_GPUTextureFormat depthFormat,
                        const Shader &vertexShader,
                        const Shader &fragmentShader) {
        VertexInputState vertexInputState = MakeVertexInputState();

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.vertex_shader = vertexShader.Get();
        pipelineCreateInfo.fragment_shader = fragmentShader.Get();
        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions =
                &vertexInputState.bufferDescription;
        pipelineCreateInfo.vertex_input_state.num_vertex_buffers = 1;
        pipelineCreateInfo.vertex_input_state.vertex_attributes = vertexInputState.attributes.data();
        pipelineCreateInfo.vertex_input_state.num_vertex_attributes =
                static_cast<Uint32>(vertexInputState.attributes.size());

        // No color target: this pipeline only ever writes to a depth
        // texture, matching SDL_gpu's depth-only render pass support
        // (num_color_targets == 0, color_target_infos == nullptr).
        pipelineCreateInfo.target_info.num_color_targets = 0;
        pipelineCreateInfo.target_info.depth_stencil_format = depthFormat;
        pipelineCreateInfo.target_info.has_depth_stencil_target = true;

        pipelineCreateInfo.depth_stencil_state.enable_depth_test = true;
        pipelineCreateInfo.depth_stencil_state.enable_depth_write = true;
        pipelineCreateInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

        pipelineCreateInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
        pipelineCreateInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;

        // Depth bias nudges every depth value written here slightly toward
        // the light before it lands in the shadow map, so a lit surface
        // doesn't shadow itself from depth-quantization rounding alone. The
        // slope factor scales with the surface's angle relative to the
        // light — grazing angles need more bias than surfaces facing the
        // light head-on. Both constants are starting points to retune by
        // eye against the actual scene.
        pipelineCreateInfo.rasterizer_state.enable_depth_bias = true;
        pipelineCreateInfo.rasterizer_state.depth_bias_constant_factor = 1.5f;
        pipelineCreateInfo.rasterizer_state.depth_bias_slope_factor = 1.75f;

        SDL_GPUGraphicsPipeline *pipeline =
                SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);

        if (!pipeline) {
            throw std::runtime_error(
                std::format("Failed to create shadow pipeline: {}", SDL_GetError()));
        }

        m_pipeline = GPUPipelineHandle(device, pipeline);
    }
}
