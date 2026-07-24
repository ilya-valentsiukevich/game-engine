#pragma once

namespace Engine {
    // Live-tunable knobs for the post-process pass (tone mapping + bloom),
    // edited through DebugUI's "Post-Processing" window. Renderer owns one
    // instance and reads it every frame in PostProcessPass.
    struct PostProcessSettings {
        // Linear multiplier applied to the HDR scene before tone mapping —
        // the same role a camera's exposure setting plays: raises or
        // lowers the whole image's brightness before the tone curve
        // compresses it into displayable range.
        float Exposure = 1.0f;

        bool BloomEnabled = true;

        // Luminance above which a pixel contributes to bloom.
        float BloomThreshold = 1.0f;

        // Flat multiplier on the blurred bright-pass result before adding
        // it back into the scene.
        float BloomIntensity = 0.5f;
    };
}
