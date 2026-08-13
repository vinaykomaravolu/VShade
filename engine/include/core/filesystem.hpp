#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace vshade::core::filesystem {

/**
 * @brief Reads an entire file as text.
 * @param path File to read.
 * @return The complete file contents.
 * @throws std::runtime_error If the file cannot be opened or read.
 */
[[nodiscard]] std::string readTextFile(const std::filesystem::path& path);

/**
 * @brief Reads an entire file as bytes.
 * @param path File to read.
 * @return The complete file contents.
 * @throws std::runtime_error If the file cannot be opened or read.
 */
[[nodiscard]] std::vector<std::uint8_t> readBinaryFile(const std::filesystem::path& path);

/**
 * @brief Returns whether a filesystem entry exists at @p path.
 * @param path Filesystem path to inspect.
 * @return True when an entry exists; otherwise false.
 */
[[nodiscard]] bool fileExists(const std::filesystem::path& path) noexcept;

/**
 * @brief Returns a path's extension, including its leading dot.
 * @param path Filesystem path whose extension is requested.
 * @return An extension such as `.png`, or an empty string when none exists.
 */
[[nodiscard]] std::string getExtension(const std::filesystem::path& path);

} // namespace vshade::core::filesystem
