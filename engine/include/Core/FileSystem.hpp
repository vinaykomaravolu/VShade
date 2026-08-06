#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace VShade::FileSystem {

[[nodiscard]] std::string ReadTextFile(const std::filesystem::path& path);
[[nodiscard]] std::vector<std::uint8_t> ReadBinaryFile(const std::filesystem::path& path);
[[nodiscard]] bool FileExists(const std::filesystem::path& path) noexcept;

// Returns the extension including its leading dot, for example ".png".
[[nodiscard]] std::string GetExtension(const std::filesystem::path& path);

} // namespace VShade::FileSystem
