#include <catch2/catch_test_macros.hpp>

#include <core/filesystem.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

class TemporaryFile final {
public:
    explicit TemporaryFile(const std::string& extension)
        : m_path(
              std::filesystem::temp_directory_path() /
              ("vshade-test-" +
               std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
               extension)
          ) {}

    ~TemporaryFile() {
        std::error_code error;
        std::filesystem::remove(m_path, error);
    }

    TemporaryFile(const TemporaryFile&) = delete;
    TemporaryFile& operator=(const TemporaryFile&) = delete;

    [[nodiscard]] const std::filesystem::path& Path() const noexcept {
        return m_path;
    }

private:
    std::filesystem::path m_path;
};

} // namespace

TEST_CASE("Text files can be read", "[filesystem]") {
    const TemporaryFile file(".txt");
    constexpr auto contents = "VShade\ntext file";

    {
        std::ofstream output(file.Path(), std::ios::binary);
        REQUIRE(output);
        output << contents;
    }

    CHECK(vshade::core::filesystem::FileExists(file.Path()));
    CHECK(vshade::core::filesystem::ReadTextFile(file.Path()) == contents);
    CHECK(vshade::core::filesystem::GetExtension(file.Path()) == ".txt");
}

TEST_CASE("Binary files preserve their bytes", "[filesystem]") {
    const TemporaryFile file(".bin");
    constexpr std::array<std::uint8_t, 5> contents{0, 1, 127, 128, 255};

    {
        std::ofstream output(file.Path(), std::ios::binary);
        REQUIRE(output);
        output.write(
            reinterpret_cast<const char*>(contents.data()),
            static_cast<std::streamsize>(contents.size())
        );
    }

    const auto result = vshade::core::filesystem::ReadBinaryFile(file.Path());
    REQUIRE(result.size() == contents.size());

    for (std::size_t index = 0; index < contents.size(); ++index) {
        CHECK(result[index] == contents[index]);
    }
}

TEST_CASE("Missing files are reported", "[filesystem]") {
    const TemporaryFile file(".missing");

    CHECK_FALSE(vshade::core::filesystem::FileExists(file.Path()));
    CHECK_THROWS_AS(
        vshade::core::filesystem::ReadTextFile(file.Path()),
        std::runtime_error
    );
}
