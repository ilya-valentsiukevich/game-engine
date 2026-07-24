//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Vertex.h>
#include <Engine/Renderer/GlmConfig.h>

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <format>
#include <stdexcept>

namespace Engine {
    namespace {
        struct VertexInputState {
            SDL_GPUVertexBufferDescription bufferDescription{};
            // Sized for the largest layout (Full: 3 attributes);
            // PositionOnly only fills index 0, None fills neither.
            std::array<SDL_GPUVertexAttribute, 3> attributes{};
            Uint32 attributeCount = 0;
            Uint32 bufferCount = 1;
        };

        VertexInputState MakeVertexInputState(Pipeline::VertexLayout layout) {
            VertexInputState state;

            if (layout == Pipeline::VertexLayout::None) {
                state.bufferCount = 0;
                return state;
            }

            state.bufferDescription.slot = 0;
            state.bufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            state.bufferDescription.instance_step_rate = 0;

            if (layout == Pipeline::VertexLayout::PositionOnly) {
                state.bufferDescription.pitch = sizeof(glm::vec3);

                state.attributes[0].location = 0;
                state.attributes[0].buffer_slot = 0;
                state.attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
                state.attributes[0].offset = 0;

                state.attributeCount = 1;
                return state;
            }

            // VertexLayout::Full — unchanged from M13.
            state.bufferDescription.pitch = sizeof(Vertex);

            state.attributes[0] = {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Vertex, Position)};
            state.attributes[1] = {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Vertex, Normal)};
            state.attributes[2] = {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Vertex, TexCoord)};
            state.attributeCount = 3;

            return state;
        }
    }

    Pipeline::Pipeline(SDL_GPUDevice *device,
                        SDL_GPUTextureFormat colorFormat,
                        std::optional<SDL_GPUTextureFormat> depthFormat,
                        const Shader &vertexShader,
                        const Shader &fragmentShader,
                        VertexLayout vertexLayout,
                        SDL_GPUCompareOp depthCompareOp,
                        bool enableDepthWrite) {
        VertexInputState vertexInputState = MakeVertexInputState(vertexLayout);

        SDL_GPUColorTargetDescription colorTargetDescription{};
        colorTargetDescription.format = colorFormat;

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.vertex_shader = vertexShader.Get();
        pipelineCreateInfo.fragment_shader = fragmentShader.Get();
        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions =
                &vertexInputState.bufferDescription;
        pipelineCreateInfo.vertex_input_state.num_vertex_buffers = vertexInputState.bufferCount;
        pipelineCreateInfo.vertex_input_state.vertex_attributes = vertexInputState.attributes.data();
        pipelineCreateInfo.vertex_input_state.num_vertex_attributes = vertexInputState.attributeCount;

        pipelineCreateInfo.target_info.color_target_descriptions = &colorTargetDescription;
        pipelineCreateInfo.target_info.num_color_targets = 1;

        if (depthFormat) {
            pipelineCreateInfo.target_info.depth_stencil_format = *depthFormat;
            pipelineCreateInfo.target_info.has_depth_stencil_target = true;

            pipelineCreateInfo.depth_stencil_state.enable_depth_test = true;
            pipelineCreateInfo.depth_stencil_state.enable_depth_write = enableDepthWrite;
            pipelineCreateInfo.depth_stencil_state.compare_op = depthCompareOp;
        }

        // The cube (skybox/precompute) is always drawn from inside, facing
        // outward — every triangle would be back-face culled from the
        // camera's point of view under the Full layout's CW convention (see
        // that branch below), so culling is simply off for these instead of
        // reasoning about a second winding order for one shape.
        if (vertexLayout == VertexLayout::Full) {
            // See M13/M12: CCW-authored glTF vertices end up CLOCKWISE on
            // screen through this engine's left-handed view/projection
            // convention (GlmConfig.h).
            pipelineCreateInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
            pipelineCreateInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
        } else {
            pipelineCreateInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        }

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
        // Depth-only shadow pipeline, always the Full vertex layout (the
        // shadow vertex shader ignores Normal/TexCoord but still reads the
        // same buffer as the main pass).
        VertexInputState vertexInputState = MakeVertexInputState(VertexLayout::Full);

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.vertex_shader = vertexShader.Get();
        pipelineCreateInfo.fragment_shader = fragmentShader.Get();
        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions =
                &vertexInputState.bufferDescription;
        pipelineCreateInfo.vertex_input_state.num_vertex_buffers = vertexInputState.bufferCount;
        pipelineCreateInfo.vertex_input_state.vertex_attributes = vertexInputState.attributes.data();
        pipelineCreateInfo.vertex_input_state.num_vertex_attributes = vertexInputState.attributeCount;

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
