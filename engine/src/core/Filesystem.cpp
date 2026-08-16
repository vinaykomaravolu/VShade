#include "core/Filesystem.hpp"

#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace vshade::core::filesystem {
namespace {

std::runtime_error readError(const std::filesystem::path& path) {
    return std::runtime_error("Failed to read file: " + path.string());
}

} // namespace

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw readError(path);
    }

    return {
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };
}

std::vector<std::uint8_t> readBinaryFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw readError(path);
    }

    const auto end = file.tellg();
    if (end < 0) {
        throw readError(path);
    }

    const auto size = static_cast<std::uintmax_t>(static_cast<std::streamoff>(end));
    if (size > std::numeric_limits<std::size_t>::max() ||
        size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
        throw readError(path);
    }

    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    file.seekg(0, std::ios::beg);

    if (!data.empty()) {
        file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!file) {
            throw readError(path);
        }
    }

    return data;
}

bool fileExists(const std::filesystem::path& path) noexcept {
    std::error_code error;
    return std::filesystem::is_regular_file(path, error);
}

std::string getExtension(const std::filesystem::path& path) {
    return path.extension().string();
}

} // namespace vshade::core::filesystem
