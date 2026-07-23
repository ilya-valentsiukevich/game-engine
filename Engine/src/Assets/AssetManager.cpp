#include <Engine/Assets/AssetManager.h>
#include <Engine/Renderer/Texture.h>

#include <array>

namespace Engine {
    void AssetManager::ReloadChanged() {
        Textures.ReloadChanged();
    }

    AssetHandle<Texture> AssetManager::GetWhiteTexture(SDL_GPUDevice *device) {
        if (!m_whiteTexture) {
            m_whiteTexture = std::make_shared<Texture>(
                device,
                std::array<unsigned char, 4>{255, 255, 255, 255},
                SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB);
        }

        return m_whiteTexture;
    }
}
