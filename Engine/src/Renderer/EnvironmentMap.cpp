#include <Engine/Renderer/EnvironmentMap.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Pipeline.h>

#include <glm/gtc/matrix_transform.hpp>

// stb_image's implementation is already compiled once, in Texture.cpp —
// including stb_image.h here only pulls in the declarations (stbi_loadf,
// stbi_image_free, stbi_failure_reason).
#include <stb_image.h>

#include <cstring>
#include <format>
#include <stdexcept>

namespace Engine {
    namespace {
        // Standard unindexed unit cube — 12 triangles, 36 vertices, no
        // shared vertices between faces (a cube's face normals aren't
        // continuous, so nothing would be gained by indexing). Winding
        // doesn't matter here: every pipeline that draws this disables
        // culling (Pipeline::VertexLayout::PositionOnly).
        constexpr std::array<glm::vec3, 36> kCubeVertices{{
            {-1.0f,  1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f},

            {-1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f},
            {-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f},

            { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f},

            {-1.0f, -1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f},

            {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f, -1.0f},

            {-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
        }};

        // The 6 face view directions/up vectors, in the fixed +X, -X, +Y,
        // -Y, +Z, -Z order SDL_gpu's layer_or_depth_plane expects for a
        // cube texture. 90-degree FOV: each face's view frustum then covers
        // exactly one face, no overlap and no gap.
        struct CubeFace {
            glm::vec3 target;
            glm::vec3 up;
        };

        // The OpenGL-reference up vector for every face (see e.g.
        // LearnOpenGL's captureViews) is correct for a Y-up framebuffer,
        // but this engine's Metal backend rasterizes into a Y-down render
        // target. Negating "up" for all 6 captures below rotates each one
        // 180 degrees around its own view direction, which is exactly what
        // a Y-down framebuffer needs to come out right side up when later
        // read back by hardware cubemap sampling — verified by eye: without
        // the negation, sky/ground were swapped within each side face, and
        // even after fixing the sides, the poles' seams with their
        // neighbors stayed off by the same 180-degree in-plane rotation
        // until this same fix was applied to them too.
        constexpr std::array<CubeFace, 6> kCubeFaces{{
            {{ 1.0f,  0.0f,  0.0f}, {0.0f,  1.0f,  0.0f}},
            {{-1.0f,  0.0f,  0.0f}, {0.0f,  1.0f,  0.0f}},
            {{ 0.0f,  1.0f,  0.0f}, {0.0f,  0.0f, -1.0f}},
            {{ 0.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}},
            {{ 0.0f,  0.0f,  1.0f}, {0.0f,  1.0f,  0.0f}},
            {{ 0.0f,  0.0f, -1.0f}, {0.0f,  1.0f,  0.0f}},
        }};

        std::array<glm::mat4, 6> MakeFaceViewProjections() {
            const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

            std::array<glm::mat4, 6> viewProjections{};
            for (int i = 0; i < 6; ++i) {
                const glm::mat4 view = glm::lookAt(glm::vec3(0.0f), kCubeFaces[i].target, kCubeFaces[i].up);
                viewProjections[i] = projection * view;
            }
            return viewProjections;
        }

        // Renders EnvironmentMap's shared cube into every face of `target`
        // at mip level `mipLevel`, pushing `viewProjections[face]` as
        // vertex uniform buffer 0 and (optionally) `pushFragmentUniform`
        // as fragment uniform buffer 0 before each face's draw call — used
        // as-is for equirect-to-cubemap/irradiance (no fragment uniform)
        // and, with a non-null pushFragmentUniform, for one mip level of
        // the prefilter pass (Roughness).
        template<typename PushFragmentUniformFn>
        void RenderCubeFaces(SDL_GPUCommandBuffer *commandBuffer,
                              const Pipeline &pipeline, const Buffer<glm::vec3> &cubeVertexBuffer,
                              SDL_GPUTexture *target, Uint32 mipLevel,
                              const std::array<glm::mat4, 6> &viewProjections,
                              SDL_GPUTexture *sourceTexture, SDL_GPUSampler *sourceSampler,
                              PushFragmentUniformFn pushFragmentUniform) {
            for (Uint32 face = 0; face < 6; ++face) {
                SDL_GPUColorTargetInfo colorTarget{};
                colorTarget.texture = target;
                colorTarget.mip_level = mipLevel;
                colorTarget.layer_or_depth_plane = face;
                colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
                colorTarget.store_op = SDL_GPU_STOREOP_STORE;

                SDL_GPURenderPass *renderPass =
                        SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, nullptr);

                SDL_BindGPUGraphicsPipeline(renderPass, pipeline.Get());

                SDL_GPUBufferBinding vertexBinding{};
                vertexBinding.buffer = cubeVertexBuffer.Get();
                vertexBinding.offset = 0;
                SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

                SDL_PushGPUVertexUniformData(
                    commandBuffer, 0, &viewProjections[face], sizeof(glm::mat4));

                pushFragmentUniform(commandBuffer);

                SDL_GPUTextureSamplerBinding sourceBinding{};
                sourceBinding.texture = sourceTexture;
                sourceBinding.sampler = sourceSampler;
                SDL_BindGPUFragmentSamplers(renderPass, 0, &sourceBinding, 1);

                SDL_DrawGPUPrimitives(renderPass, cubeVertexBuffer.Count(), 1, 0, 0);

                SDL_EndGPURenderPass(renderPass);
            }
        }

        void NoFragmentUniform(SDL_GPUCommandBuffer *) {
        }
    }

    EnvironmentMap::EnvironmentMap(SDL_GPUDevice *device, const std::filesystem::path &hdrPath)
        : m_cubeVertexBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, std::span(kCubeVertices)) {
        // --- HDR sampler: shared by every precompute stage below and by
        // Renderer's runtime skybox/IBL sampling. CLAMP_TO_EDGE avoids the
        // equirect-to-cubemap pass wrapping at the panorama's poles;
        // trilinear mip filtering is what lets Mesh.frag.msl pick a
        // fractional roughness-driven LOD on the prefiltered map later.
        SDL_GPUSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.min_filter = SDL_GPU_FILTER_LINEAR;
        samplerCreateInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
        samplerCreateInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        samplerCreateInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerCreateInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerCreateInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        m_hdrSampler = GPUSamplerHandle(device, SDL_CreateGPUSampler(device, &samplerCreateInfo));

        if (!m_hdrSampler) {
            throw std::runtime_error(
                std::format("Failed to create HDR sampler: {}", SDL_GetError()));
        }

        // --- Load the equirectangular HDRI as floating-point pixels (not
        // stbi_load's 8-bit path Texture.cpp uses — an HDRI's whole point
        // is values above 1.0, which UNORM8 cannot represent at all).
        int width = 0;
        int height = 0;
        int sourceChannels = 0;

        float *hdrPixels = stbi_loadf(hdrPath.string().c_str(), &width, &height, &sourceChannels, 4);

        if (!hdrPixels) {
            throw std::runtime_error(
                std::format("Failed to load HDRI ({}): {}", hdrPath.string(), stbi_failure_reason()));
        }

        SDL_GPUTextureCreateInfo equirectCreateInfo{};
        equirectCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
        equirectCreateInfo.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
        equirectCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        equirectCreateInfo.width = static_cast<Uint32>(width);
        equirectCreateInfo.height = static_cast<Uint32>(height);
        equirectCreateInfo.layer_count_or_depth = 1;
        equirectCreateInfo.num_levels = 1;
        equirectCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

        GPUTextureHandle equirectTexture(device, SDL_CreateGPUTexture(device, &equirectCreateInfo));

        if (!equirectTexture) {
            stbi_image_free(hdrPixels);
            throw std::runtime_error(std::format("Failed to create HDRI texture: {}", SDL_GetError()));
        }

        {
            const Uint32 pixelDataSize =
                    static_cast<Uint32>(width) * static_cast<Uint32>(height) * 4 * sizeof(float);

            SDL_GPUTransferBufferCreateInfo transferCreateInfo{};
            transferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferCreateInfo.size = pixelDataSize;

            GPUTransferBufferHandle transferBuffer(
                device, SDL_CreateGPUTransferBuffer(device, &transferCreateInfo));

            void *mapped = SDL_MapGPUTransferBuffer(device, transferBuffer.Get(), false);
            std::memcpy(mapped, hdrPixels, pixelDataSize);
            SDL_UnmapGPUTransferBuffer(device, transferBuffer.Get());

            stbi_image_free(hdrPixels);

            SDL_GPUCommandBuffer *uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
            SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

            SDL_GPUTextureTransferInfo source{};
            source.transfer_buffer = transferBuffer.Get();
            source.pixels_per_row = static_cast<Uint32>(width);
            source.rows_per_layer = static_cast<Uint32>(height);

            SDL_GPUTextureRegion destination{};
            destination.texture = equirectTexture.Get();
            destination.w = static_cast<Uint32>(width);
            destination.h = static_cast<Uint32>(height);
            destination.d = 1;

            SDL_UploadToGPUTexture(copyPass, &source, &destination, false);
            SDL_EndGPUCopyPass(copyPass);
            SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);
        }

        // --- Shared face view-projections and shaders for the three
        // cube-render precompute stages below.
        const std::array<glm::mat4, 6> faceViewProjections = MakeFaceViewProjections();

        const Shader cubeVertexShader(
            device, "Assets/Shaders/Compiled/Cube.vert.msl", SDL_GPU_SHADERSTAGE_VERTEX, 1);

        // --- Stage 1: equirectangular HDRI -> base environment cubemap.
        m_environmentCubemap = std::make_unique<CubemapTexture>(device, kEnvironmentSize, 1, kHdrFormat);

        {
            const Shader equirectFragmentShader(
                device, "Assets/Shaders/Compiled/EquirectToCubemap.frag.msl",
                SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);

            const Pipeline equirectPipeline(
                device, kHdrFormat, std::nullopt, cubeVertexShader, equirectFragmentShader,
                Pipeline::VertexLayout::PositionOnly);

            SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);
            RenderCubeFaces(
                commandBuffer, equirectPipeline, m_cubeVertexBuffer,
                m_environmentCubemap->Get(), 0, faceViewProjections,
                equirectTexture.Get(), m_hdrSampler.Get(), NoFragmentUniform);
            SDL_SubmitGPUCommandBuffer(commandBuffer);
        }

        // --- Stage 2: base environment cubemap -> diffuse irradiance cubemap.
        m_irradianceCubemap = std::make_unique<CubemapTexture>(device, kIrradianceSize, 1, kHdrFormat);

        {
            const Shader irradianceFragmentShader(
                device, "Assets/Shaders/Compiled/IrradianceConvolution.frag.msl",
                SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);

            const Pipeline irradiancePipeline(
                device, kHdrFormat, std::nullopt, cubeVertexShader, irradianceFragmentShader,
                Pipeline::VertexLayout::PositionOnly);

            SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);
            RenderCubeFaces(
                commandBuffer, irradiancePipeline, m_cubeVertexBuffer,
                m_irradianceCubemap->Get(), 0, faceViewProjections,
                m_environmentCubemap->Get(), m_hdrSampler.Get(), NoFragmentUniform);
            SDL_SubmitGPUCommandBuffer(commandBuffer);
        }

        // --- Stage 3: base environment cubemap -> prefiltered specular
        // cubemap, one mip level per roughness value.
        m_prefilteredCubemap = std::make_unique<CubemapTexture>(
            device, kPrefilterSize, kPrefilterMipLevels, kHdrFormat);

        {
            const Shader prefilterFragmentShader(
                device, "Assets/Shaders/Compiled/Prefilter.frag.msl",
                SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);

            const Pipeline prefilterPipeline(
                device, kHdrFormat, std::nullopt, cubeVertexShader, prefilterFragmentShader,
                Pipeline::VertexLayout::PositionOnly);

            for (Uint32 mip = 0; mip < kPrefilterMipLevels; ++mip) {
                const float roughness = static_cast<float>(mip) / static_cast<float>(kPrefilterMipLevels - 1);

                SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);
                RenderCubeFaces(
                    commandBuffer, prefilterPipeline, m_cubeVertexBuffer,
                    m_prefilteredCubemap->Get(), mip, faceViewProjections,
                    m_environmentCubemap->Get(), m_hdrSampler.Get(),
                    [roughness](SDL_GPUCommandBuffer *cmd) {
                        SDL_PushGPUFragmentUniformData(cmd, 0, &roughness, sizeof(roughness));
                    });
                SDL_SubmitGPUCommandBuffer(commandBuffer);
            }
        }

        // --- Stage 4: BRDF integration LUT — no cube, no environment
        // sample, pure math over a fullscreen triangle.
        {
            SDL_GPUTextureCreateInfo lutCreateInfo{};
            lutCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
            lutCreateInfo.format = kBRDFLutFormat;
            lutCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
            lutCreateInfo.width = kBRDFLutSize;
            lutCreateInfo.height = kBRDFLutSize;
            lutCreateInfo.layer_count_or_depth = 1;
            lutCreateInfo.num_levels = 1;
            lutCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

            m_brdfLutTexture = GPUTextureHandle(device, SDL_CreateGPUTexture(device, &lutCreateInfo));

            if (!m_brdfLutTexture) {
                throw std::runtime_error(std::format("Failed to create BRDF LUT texture: {}", SDL_GetError()));
            }

            const Shader lutVertexShader(
                device, "Assets/Shaders/Compiled/Fullscreen.vert.msl",
                SDL_GPU_SHADERSTAGE_VERTEX, 0);
            const Shader lutFragmentShader(
                device, "Assets/Shaders/Compiled/BRDFIntegration.frag.msl",
                SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);

            const Pipeline lutPipeline(
                device, kBRDFLutFormat, std::nullopt, lutVertexShader, lutFragmentShader,
                Pipeline::VertexLayout::None);

            SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);

            SDL_GPUColorTargetInfo colorTarget{};
            colorTarget.texture = m_brdfLutTexture.Get();
            colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
            colorTarget.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass *renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, nullptr);
            SDL_BindGPUGraphicsPipeline(renderPass, lutPipeline.Get());
            SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
            SDL_EndGPURenderPass(renderPass);

            SDL_SubmitGPUCommandBuffer(commandBuffer);
        }

        // equirectTexture's transfer buffer, and every Shader/Pipeline
        // declared inside the blocks above, release themselves here via
        // RAII — none of them are needed once their one-shot bake is done.
    }

    void EnvironmentMap::DrawCube(SDL_GPURenderPass *renderPass) const {
        SDL_GPUBufferBinding vertexBinding{};
        vertexBinding.buffer = m_cubeVertexBuffer.Get();
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

        SDL_DrawGPUPrimitives(renderPass, m_cubeVertexBuffer.Count(), 1, 0, 0);
    }
}
