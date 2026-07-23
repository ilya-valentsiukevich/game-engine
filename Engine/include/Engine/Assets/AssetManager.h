#pragma once

#include <Engine/Assets/AssetCache.h>

#include <SDL3/SDL_gpu.h>

namespace Engine {
    class Texture;
    class Model;

    // Owns one AssetCache per asset type this engine knows how to load — a
    // single instance lives on Scene and is threaded down to whatever needs
    // to load assets by path (see Model's constructor).
    struct AssetManager {
        AssetCache<Texture> Textures;
        AssetCache<Model> Models;

        // Re-stats every cached asset and reloads any whose file changed.
        // Models don't support in-place reload yet — only Textures do.
        void ReloadChanged();

        // Lazily creates (once) and returns a shared 1x1 opaque white
        // texture — the fallback Model uses for a glTF primitive with no
        // base color texture of its own, so baseColorFactor can still tint
        // it via multiplication in the fragment shader. Not routed through
        // AssetCache<Texture>: that cache keys and hot-reload-watches by a
        // real file path, which this texture doesn't have.
        AssetHandle<Texture> GetWhiteTexture(SDL_GPUDevice *device);

    private:
        AssetHandle<Texture> m_whiteTexture;
    };
}
