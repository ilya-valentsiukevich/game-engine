#pragma once

// SDL_gpu's clip space is left-handed with depth in [0, 1] (D3D12/Metal
// convention), unlike glm's OpenGL-style defaults. Must be defined before
// the first glm include anywhere in the project — centralized here and
// included by every header that touches glm, so every translation unit
// sees the same configuration (mixing configurations across translation
// units is an ODR violation: glm conditionally compiles different function
// bodies based on these macros).
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
