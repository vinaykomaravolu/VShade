#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace VShade::FileSystem {

/**
 * @brief Reads an entire file as text.
 * @param path File to read.
 * @return The complete file contents.
 * @throws std::runtime_error If the file cannot be opened or read.
 */
[[nodiscard]] std::string ReadTextFile(const std::filesystem::path& path);

/**
 * @brief Reads an entire file as bytes.
 * @param path File to read.
 * @return The complete file contents.
 * @throws std::runtime_error If the file cannot be opened or read.
 */
[[nodiscard]] std::vector<std::uint8_t> ReadBinaryFile(const std::filesystem::path& path);

/** @brief Returns whether a filesystem entry exists at @p path. */
[[nodiscard]] bool FileExists(const std::filesystem::path& path) noexcept;

/**
 * @brief Returns a path's extension, including its leading dot.
 * @return An extension such as `.png`, or an empty string when none exists.
 */
[[nodiscard]] std::string GetExtension(const std::filesystem::path& path);

} // namespace VShade::FileSystem
