#pragma once

#include <Engine/Assets/AssetCache.h>

namespace Engine {
    class Texture;
    class Model;

    // Owns one AssetCache per asset type this engine knows how to load — a
    // single instance lives on Renderer and is threaded down to whatever
    // needs to load assets by path (see Model's constructor).
    struct AssetManager {
        AssetCache<Texture> Textures;
        AssetCache<Model> Models;

        // Re-stats every cached asset and reloads any whose file changed.
        // Models don't support in-place reload yet — only Textures do.
        void ReloadChanged();
    };
}
