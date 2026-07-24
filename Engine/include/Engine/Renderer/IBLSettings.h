#pragma once

namespace Engine {
    // Live-tunable knobs for image-based lighting's ambient term
    // (Mesh.frag.msl), edited through DebugUI's "IBL" window so their
    // effect can be judged by eye without touching the shader and
    // relaunching. Renderer owns one instance and pushes it into
    // LightUniformBlock.IBLParams every frame.
    struct IBLSettings {
        // Master switch for both the visible skybox background and the IBL
        // ambient term. Off: MainPass skips SkyboxPass (the plain clear
        // color shows through instead) and Mesh.frag.msl falls back to a
        // flat ambient formula (SunParams.x * SunColor * albedo) instead of
        // sampling the environment. EnvironmentMap itself still loads and
        // bakes at startup either way — this only gates how the result
        // gets used, so flipping it back on doesn't need a relaunch.
        // Defaults off: ambient specular was reflecting cloud detail off
        // per-texel roughness variation on the low-poly character meshes
        // that wasn't visible before an environment existed to reflect.
        bool Enabled = false;

        // Floors the roughness used only for the ambient specular lookup
        // (prefiltered map + BRDF LUT) — keeps a material's real, possibly
        // low, roughness for direct-light highlights, while softening how
        // sharply it mirrors the whole prefiltered sky dome.
        float MinAmbientRoughness = 0.35f;

        // Mip bias for the prefiltered specular sample: roughness * this
        // picks the LOD. Must track EnvironmentMap::kPrefilterMipLevels - 1
        // — mismatched, reflections either never reach the blurriest mip
        // (too low a value here) or clamp to it too early (too high).
        float MaxReflectionLod = 4.0f;

        // Flat multiplier on the whole ambient term (diffuse + specular),
        // applied after everything else — a quick way to tell "IBL is too
        // strong overall" apart from "one specific term is too strong".
        float AmbientIntensity = 1.0f;
    };
}
