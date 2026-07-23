//
// Created by Ilya Valentsiukevich on 23/07/2026.
//
#pragma once

#include <Engine/Renderer/Vertex.h>

#include <SDL3/SDL_stdinc.h>
#include <array>

namespace Engine {
    // Unit cube (half-extent 0.5) centered at the origin. 24 vertices (4 per
    // face) for the same reason M2 needed them for flat per-face color: a
    // shared corner vertex can't have two different UV origins depending on
    // which face it belongs to. Every face uses the identical UV pattern
    // below because every face's 4 vertices are wound the same way
    // (bottom-left, bottom-right, top-right, top-left in the face's own
    // local frame) — see M4-textures.md.
    inline constexpr std::array<Vertex, 24> kCubeVertices{
        // +Z (front)
        Vertex{{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f}},
        Vertex{{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f}},
        Vertex{{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f}},
        Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},
        // -Z (back)
        Vertex{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
        Vertex{{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}},
        Vertex{{-0.5f, 0.5f, -0.5f}, {1.0f, 0.0f}},
        Vertex{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}},
        // -X (left)
        Vertex{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
        Vertex{{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f}},
        Vertex{{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f}},
        Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}},
        // +X (right)
        Vertex{{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f}},
        Vertex{{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}},
        Vertex{{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f}},
        Vertex{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},
        // +Y (top)
        Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}},
        Vertex{{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}},
        Vertex{{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f}},
        Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}},
        // -Y (bottom)
        Vertex{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
        Vertex{{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}},
        Vertex{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}},
        Vertex{{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},
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
