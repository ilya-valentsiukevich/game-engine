#include <Engine/Assets/AssetManager.h>

namespace Engine {
    void AssetManager::ReloadChanged() {
        Textures.ReloadChanged();
    }
}
