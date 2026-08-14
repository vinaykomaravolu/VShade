#include <catch2/catch_test_macros.hpp>

#include <core/filesystem.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

[[nodiscard]] std::filesystem::path outputPath(const std::string& filename) {
    const std::filesystem::path directory{VSHADE_FILESYSTEM_OUTPUT_DIR};
    std::filesystem::create_directories(directory);
    return directory / filename;
}

} // namespace

TEST_CASE("Text files can be read", "[filesystem]") {
    const std::filesystem::path path = outputPath("text-file.txt");
    constexpr auto contents = "VShade\ntext file";

    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        REQUIRE(output);
        output << contents;
    }

    CHECK(vshade::core::filesystem::fileExists(path));
    CHECK(vshade::core::filesystem::readTextFile(path) == contents);
    CHECK(vshade::core::filesystem::getExtension(path) == ".txt");
}

TEST_CASE("Binary files preserve their bytes", "[filesystem]") {
    const std::filesystem::path path = outputPath("binary-file.bin");
    constexpr std::array<std::uint8_t, 5> contents{0, 1, 127, 128, 255};

    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        REQUIRE(output);
        output.write(
            reinterpret_cast<const char*>(contents.data()),
            static_cast<std::streamsize>(contents.size())
        );
    }

    const auto result = vshade::core::filesystem::readBinaryFile(path);
    REQUIRE(result.size() == contents.size());

    for (std::size_t index = 0; index < contents.size(); ++index) {
        CHECK(result[index] == contents[index]);
    }
}

TEST_CASE("Missing files are reported", "[filesystem]") {
    const std::filesystem::path path = outputPath("intentionally-missing.file");

    CHECK_FALSE(vshade::core::filesystem::fileExists(path));
    CHECK_THROWS_AS(
        vshade::core::filesystem::readTextFile(path),
        std::runtime_error
    );
}
