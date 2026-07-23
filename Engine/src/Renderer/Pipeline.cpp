//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Vertex.h>

#include <cstddef>
#include <format>
#include <stdexcept>

namespace Engine {
    Pipeline::Pipeline(SDL_GPUDevice *device,
                        SDL_GPUTextureFormat colorFormat,
                        SDL_GPUTextureFormat depthFormat,
                        const Shader &vertexShader,
                        const Shader &fragmentShader) {
        SDL_GPUColorTargetDescription colorTargetDescription{};
        colorTargetDescription.format = colorFormat;

        SDL_GPUVertexBufferDescription vertexBufferDescription{};
        vertexBufferDescription.slot = 0;
        vertexBufferDescription.pitch = sizeof(Vertex);
        vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vertexBufferDescription.instance_step_rate = 0;

        SDL_GPUVertexAttribute vertexAttributes[3]{};
        vertexAttributes[0].location = 0;
        vertexAttributes[0].buffer_slot = 0;
        vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertexAttributes[0].offset = offsetof(Vertex, Position);

        vertexAttributes[1].location = 1;
        vertexAttributes[1].buffer_slot = 0;
        vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertexAttributes[1].offset = offsetof(Vertex, Normal);

        vertexAttributes[2].location = 2;
        vertexAttributes[2].buffer_slot = 0;
        vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertexAttributes[2].offset = offsetof(Vertex, TexCoord);

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.vertex_shader = vertexShader.Get();
        pipelineCreateInfo.fragment_shader = fragmentShader.Get();
        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions =
                &vertexBufferDescription;
        pipelineCreateInfo.vertex_input_state.num_vertex_buffers = 1;
        pipelineCreateInfo.vertex_input_state.vertex_attributes = vertexAttributes;
        pipelineCreateInfo.vertex_input_state.num_vertex_attributes = 3;

        pipelineCreateInfo.target_info.color_target_descriptions =
                &colorTargetDescription;
        pipelineCreateInfo.target_info.num_color_targets = 1;
        pipelineCreateInfo.target_info.depth_stencil_format = depthFormat;
        pipelineCreateInfo.target_info.has_depth_stencil_target = true;

        pipelineCreateInfo.depth_stencil_state.enable_depth_test = true;
        pipelineCreateInfo.depth_stencil_state.enable_depth_write = true;
        pipelineCreateInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

        SDL_GPUGraphicsPipeline *pipeline =
                SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);

        if (!pipeline) {
            throw std::runtime_error(
                std::format("Failed to create pipeline: {}", SDL_GetError()));
        }

        m_pipeline = GPUPipelineHandle(device, pipeline);
    }
}
