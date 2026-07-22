//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#include <Engine/Assets/ShaderLoader/ShaderLoader.h>

#include <fstream>
#include <stdexcept>

std::vector<std::byte> ShaderLoader::LoadBinary(
    const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file)
        throw std::runtime_error("Failed to open shader: " + path.string());

    const auto size = file.tellg();

    std::vector<std::byte> data(size);

    file.seekg(0);

    file.read(
        reinterpret_cast<char *>(data.data()),
        size);

    return data;
}
