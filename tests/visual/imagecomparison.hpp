#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace vshade::tests::visual {

/** @brief Four-channel 8-bit image stored in top-to-bottom row order. */
struct Image {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> pixels;
};

/** @brief Configurable thresholds used to accept small rendering differences. */
struct ComparisonTolerance {
    /** @brief Per-channel difference above which a pixel is considered outside tolerance. */
    std::uint8_t channelError = 4;
    /** @brief Maximum allowed mean absolute channel error across the image. */
    double meanError = 1.0;
    /** @brief Maximum allowed error for any individual channel. */
    std::uint8_t maximumError = 255;
    /** @brief Maximum percentage of pixels allowed outside channelError. */
    double pixelsOutsidePercentage = 1.0;
};

/** @brief Detailed statistics produced by an image comparison. */
struct ComparisonResult {
    bool passed = false;
    bool dimensionsMatch = false;
    double meanPixelError = 0.0;
    std::uint8_t maximumPixelError = 0;
    std::uint64_t pixelsOutsideTolerance = 0;
    std::uint64_t totalPixels = 0;
    double pixelsOutsidePercentage = 0.0;

    /** @brief Formats all comparison statistics for a test failure message. */
    [[nodiscard]] std::string summary() const;
};

/** @brief Loads a PNG and converts it to four 8-bit channels. */
[[nodiscard]] Image loadPng(const std::filesystem::path& path);

/** @brief Writes a four-channel image as a PNG. */
void writePng(const std::filesystem::path& path, const Image& image);

/** @brief Compares two images without requiring exact byte equality. */
[[nodiscard]] ComparisonResult compareImages(
    const Image& actual,
    const Image& expected,
    const ComparisonTolerance& tolerance
);

/**
 * @brief Compares against a golden PNG and writes persistent test artifacts.
 *
 * actual.png is written on every run. A failed comparison also writes
 * expected.png and difference.png. Fixed filenames make later runs overwrite
 * older artifacts, and the golden image is never modified.
 */
[[nodiscard]] ComparisonResult compareAgainstGolden(
    const Image& actual,
    const std::filesystem::path& goldenPath,
    const std::filesystem::path& outputDirectory,
    const ComparisonTolerance& tolerance
);

} // namespace vshade::tests::visual
