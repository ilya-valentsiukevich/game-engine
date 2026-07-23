//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/Vertex.h>

#include <SDL3/SDL_stdinc.h>
#include <array>

namespace Engine {
    // Unit cube (half-extent 0.5) centered at the origin. Flat per-face
    // shading needs 4 unique vertices per face (24 total) since a shared
    // corner vertex would need a different color depending on which face
    // it belongs to.
    inline constexpr std::array<Vertex, 24> kCubeVertices{
        // +Z (front) — red
        Vertex{{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
        Vertex{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
        Vertex{{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
        Vertex{{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
        // -Z (back) — green
        Vertex{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        Vertex{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        Vertex{{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        // -X (left) — blue
        Vertex{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        Vertex{{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        // +X (right) — yellow
        Vertex{{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
        Vertex{{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
        Vertex{{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
        Vertex{{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
        // +Y (top) — cyan
        Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
        Vertex{{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
        Vertex{{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
        Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
        // -Y (bottom) — magenta
        Vertex{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
        Vertex{{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
        Vertex{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},
        Vertex{{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},
    };

    inline constexpr std::array<Uint16, 36> kCubeIndices{
        0, 1, 2, 0, 2, 3, // front
        4, 5, 6, 4, 6, 7, // back
        8, 9, 10, 8, 10, 11, // left
        12, 13, 14, 12, 14, 15, // right
        16, 17, 18, 16, 18, 19, // top
        20, 21, 22, 20, 22, 23, // bottom
    };
}
