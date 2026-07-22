//
// Created by Ilya Valentsiukevich on 23/07/2026.
//

#pragma once
#include <filesystem>
#include <vector>

class ShaderLoader {
public:
    static std::vector<std::byte> LoadBinary(
        const std::filesystem::path &path);
};
