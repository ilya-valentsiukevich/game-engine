//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#include <Engine/Renderer/Renderer.h>
#include <Engine/Renderer/GPUDevice.h>
#include <Engine/Renderer/Vertex.h>
#include <Engine/Window/Window.h>
#include <Engine/Assets/ShaderLoader/ShaderLoader.h>

#include <cstring>
#include <format>

Renderer::Renderer(Window &window)
    : m_window(&window), m_device(std::make_unique<GPUDevice>()) {
    SDL_Log(
        "GPU Driver: %s",
        SDL_GetGPUDeviceDriver(m_device->Get())
    );

    if (!SDL_ClaimWindowForGPUDevice(
        m_device->Get(),
        m_window->GetNativeWindow())) {
        SDL_Log(
            "Failed to claim window: %s",
            SDL_GetError());

        throw std::runtime_error(
            std::format("Failed to claim window: {}", SDL_GetError()));
    }

    if (!LoadShaders()) {
        throw std::runtime_error("Failed to load shaders");
    }

    if (!CreatePipeline()) {
        throw std::runtime_error("Failed to create pipeline");
    }

    if (!CreateVertexBuffer()) {
        throw std::runtime_error("Failed to create vertex buffer");
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

    m_renderPass = SDL_BeginGPURenderPass(
        m_commandBuffer,
        &colorTarget,
        1,
        nullptr);

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

    // m_vertexBuffer/m_pipeline/m_vertexShader/m_fragmentShader release
    // themselves via GPUResource's destructor (declared after m_device,
    // so they run first). m_device itself is destroyed last.
    SDL_ReleaseWindowFromGPUDevice(
        m_device->Get(),
        m_window->GetNativeWindow());
}

void Renderer::Render() {
    if (!m_renderPass)
        return;

    SDL_BindGPUGraphicsPipeline(m_renderPass, m_pipeline.Get());

    SDL_GPUBufferBinding binding{};
    binding.buffer = m_vertexBuffer.Get();
    binding.offset = 0;

    SDL_BindGPUVertexBuffers(m_renderPass, 0, &binding, 1);

    SDL_DrawGPUPrimitives(m_renderPass, 3, 1, 0, 0);
}

GPUShaderHandle Renderer::CreateShader(
    const std::filesystem::path &path,
    SDL_GPUShaderStage stage) {
    const auto code = ShaderLoader::LoadBinary(path);

    SDL_GPUShaderCreateInfo createInfo{};
    createInfo.code = reinterpret_cast<const Uint8 *>(code.data());
    createInfo.code_size = code.size();
    createInfo.entrypoint = "main0";
    createInfo.format = SDL_GPU_SHADERFORMAT_MSL;
    createInfo.stage = stage;
    createInfo.num_samplers = 0;
    createInfo.num_uniform_buffers = 0;
    createInfo.num_storage_buffers = 0;
    createInfo.num_storage_textures = 0;

    SDL_GPUShader *shader = SDL_CreateGPUShader(m_device->Get(), &createInfo);

    if (!shader) {
        SDL_Log(
            "Failed to create shader (%s): %s",
            path.string().c_str(),
            SDL_GetError());
    }

    return GPUShaderHandle(m_device->Get(), shader);
}

bool Renderer::LoadShaders() {
    m_vertexShader = CreateShader(
        "Assets/Shaders/Compiled/Triangle.vert.msl",
        SDL_GPU_SHADERSTAGE_VERTEX);

    if (!m_vertexShader)
        return false;

    m_fragmentShader = CreateShader(
        "Assets/Shaders/Compiled/Triangle.frag.msl",
        SDL_GPU_SHADERSTAGE_FRAGMENT);

    if (!m_fragmentShader)
        return false;

    return true;
}

bool Renderer::CreatePipeline() {
    SDL_GPUColorTargetDescription colorTargetDescription{};
    colorTargetDescription.format = SDL_GetGPUSwapchainTextureFormat(
        m_device->Get(),
        m_window->GetNativeWindow());

    SDL_GPUVertexBufferDescription vertexBufferDescription{};
    vertexBufferDescription.slot = 0;
    vertexBufferDescription.pitch = sizeof(Vertex);
    vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescription.instance_step_rate = 0;

    SDL_GPUVertexAttribute vertexAttributes[2]{};
    vertexAttributes[0].location = 0;
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[0].offset = offsetof(Vertex, Position);

    vertexAttributes[1].location = 1;
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[1].offset = offsetof(Vertex, Color);

    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.vertex_shader = m_vertexShader.Get();
    pipelineCreateInfo.fragment_shader = m_fragmentShader.Get();
    pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions =
            &vertexBufferDescription;
    pipelineCreateInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineCreateInfo.vertex_input_state.vertex_attributes = vertexAttributes;
    pipelineCreateInfo.vertex_input_state.num_vertex_attributes = 2;

    pipelineCreateInfo.target_info.color_target_descriptions =
            &colorTargetDescription;
    pipelineCreateInfo.target_info.num_color_targets = 1;

    SDL_GPUGraphicsPipeline *pipeline =
            SDL_CreateGPUGraphicsPipeline(m_device->Get(), &pipelineCreateInfo);

    m_pipeline = GPUPipelineHandle(m_device->Get(), pipeline);

    // Once the pipeline is built, the shader modules are no longer needed.
    m_vertexShader.Reset();
    m_fragmentShader.Reset();

    if (!m_pipeline) {
        SDL_Log("Failed to create pipeline: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool Renderer::CreateVertexBuffer() {
    constexpr Vertex vertices[3] = {
        {{0.0f, 0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
    };

    constexpr Uint32 size = sizeof(vertices);

    SDL_GPUBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferCreateInfo.size = size;

    m_vertexBuffer = GPUBufferHandle(
        m_device->Get(),
        SDL_CreateGPUBuffer(m_device->Get(), &bufferCreateInfo));

    if (!m_vertexBuffer) {
        SDL_Log("Failed to create vertex buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferCreateInfo{};
    transferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferCreateInfo.size = size;

    GPUTransferBufferHandle transferBuffer(
        m_device->Get(),
        SDL_CreateGPUTransferBuffer(m_device->Get(), &transferCreateInfo));

    if (!transferBuffer) {
        SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
        return false;
    }

    void *mapped = SDL_MapGPUTransferBuffer(m_device->Get(), transferBuffer.Get(), false);

    if (!mapped) {
        SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
        return false;
    }

    std::memcpy(mapped, vertices, size);
    SDL_UnmapGPUTransferBuffer(m_device->Get(), transferBuffer.Get());

    SDL_GPUCommandBuffer *uploadCommandBuffer =
            SDL_AcquireGPUCommandBuffer(m_device->Get());

    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

    SDL_GPUTransferBufferLocation source{};
    source.transfer_buffer = transferBuffer.Get();
    source.offset = 0;

    SDL_GPUBufferRegion destination{};
    destination.buffer = m_vertexBuffer.Get();
    destination.offset = 0;
    destination.size = size;

    SDL_UploadToGPUBuffer(copyPass, &source, &destination, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

    // transferBuffer releases itself here (RAII), no manual call needed.
    return true;
}
