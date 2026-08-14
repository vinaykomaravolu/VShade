#if defined(_MSC_VER)
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include "imagecomparison.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>

#define STBI_FAILURE_USERMSG
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace vshade::tests::visual {
namespace {

constexpr std::size_t channelCount = 4;

std::size_t expectedByteCount(const Image& image) {
    if (image.width == 0 || image.height == 0) {
        throw std::invalid_argument("Image dimensions must be greater than zero");
    }

    const auto width = static_cast<std::size_t>(image.width);
    const auto height = static_cast<std::size_t>(image.height);
    if (height > std::numeric_limits<std::size_t>::max() / width) {
        throw std::overflow_error("Image dimensions are too large");
    }
    const std::size_t pixelCount = width * height;
    if (pixelCount > std::numeric_limits<std::size_t>::max() / channelCount) {
        throw std::overflow_error("Image byte count is too large");
    }
    return pixelCount * channelCount;
}

void validateImage(const Image& image) {
    if (image.pixels.size() != expectedByteCount(image)) {
        throw std::invalid_argument("Image pixel data does not match its dimensions");
    }
}

std::uint8_t absoluteDifference(const std::uint8_t first, const std::uint8_t second) {
    return static_cast<std::uint8_t>(std::abs(static_cast<int>(first) - static_cast<int>(second)));
}

Image differenceImage(const Image& actual, const Image& expected) {
    const std::uint32_t width = std::max(actual.width, expected.width);
    const std::uint32_t height = std::max(actual.height, expected.height);
    Image difference{width, height, {}};
    difference.pixels.resize(expectedByteCount(difference), 0);

    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t outputIndex =
                (static_cast<std::size_t>(y) * width + x) * channelCount;
            const bool hasActual = x < actual.width && y < actual.height;
            const bool hasExpected = x < expected.width && y < expected.height;

            if (!hasActual || !hasExpected) {
                difference.pixels[outputIndex] = 255;
                difference.pixels[outputIndex + 1] = 0;
                difference.pixels[outputIndex + 2] = 255;
                difference.pixels[outputIndex + 3] = 255;
                continue;
            }

            const std::size_t actualIndex =
                (static_cast<std::size_t>(y) * actual.width + x) * channelCount;
            const std::size_t expectedIndex =
                (static_cast<std::size_t>(y) * expected.width + x) * channelCount;
            for (std::size_t channel = 0; channel < 3; ++channel) {
                const auto error = absoluteDifference(
                    actual.pixels[actualIndex + channel],
                    expected.pixels[expectedIndex + channel]
                );
                difference.pixels[outputIndex + channel] = static_cast<std::uint8_t>(
                    std::min(static_cast<unsigned int>(error) * 4U, 255U)
                );
            }
            difference.pixels[outputIndex + 3] = 255;
        }
    }

    return difference;
}

} // namespace

std::string ComparisonResult::summary() const {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(4)
           << "dimensions match: " << (dimensionsMatch ? "yes" : "no")
           << ", mean pixel error: " << meanPixelError
           << ", maximum pixel error: " << static_cast<unsigned int>(maximumPixelError)
           << ", pixels outside tolerance: " << pixelsOutsideTolerance
           << '/' << totalPixels
           << " (" << pixelsOutsidePercentage << "%)";
    return stream.str();
}

Image loadPng(const std::filesystem::path& path) {
    int width = 0;
    int height = 0;
    int sourceChannels = 0;
    using StbiPixels = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;
    StbiPixels data(
        stbi_load(path.string().c_str(), &width, &height, &sourceChannels, 4),
        &stbi_image_free
    );
    if (!data) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error(
            "Failed to load PNG '" + path.string() + "': " +
            (reason ? reason : "unknown stb_image error")
        );
    }
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("PNG has invalid dimensions: " + path.string());
    }

    const Image dimensions{
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
        {}
    };
    const std::size_t byteCount = expectedByteCount(dimensions);
    return {
        dimensions.width,
        dimensions.height,
        std::vector<std::uint8_t>(data.get(), data.get() + byteCount)
    };
}

void writePng(const std::filesystem::path& path, const Image& image) {
    validateImage(image);
    if (image.width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        image.height > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        static_cast<std::size_t>(image.width) * channelCount >
            static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::overflow_error("Image dimensions are too large for the PNG writer");
    }
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    const int stride = static_cast<int>(static_cast<std::size_t>(image.width) * channelCount);
    const int result = stbi_write_png(
        path.string().c_str(),
        static_cast<int>(image.width),
        static_cast<int>(image.height),
        static_cast<int>(channelCount),
        image.pixels.data(),
        stride
    );
    if (result == 0) {
        throw std::runtime_error("Failed to write PNG: " + path.string());
    }
}

ComparisonResult compareImages(
    const Image& actual,
    const Image& expected,
    const ComparisonTolerance& tolerance
) {
    validateImage(actual);
    validateImage(expected);
    if (tolerance.meanError < 0.0 || tolerance.pixelsOutsidePercentage < 0.0) {
        throw std::invalid_argument("Image comparison tolerances must not be negative");
    }

    ComparisonResult result;
    result.dimensionsMatch = actual.width == expected.width && actual.height == expected.height;
    result.totalPixels = std::max(
        static_cast<std::uint64_t>(actual.width) * actual.height,
        static_cast<std::uint64_t>(expected.width) * expected.height
    );

    if (!result.dimensionsMatch) {
        result.meanPixelError = 255.0;
        result.maximumPixelError = 255;
        result.pixelsOutsideTolerance = result.totalPixels;
        result.pixelsOutsidePercentage = 100.0;
        return result;
    }

    std::uint64_t totalChannelError = 0;
    for (std::uint64_t pixel = 0; pixel < result.totalPixels; ++pixel) {
        bool outsideTolerance = false;
        const std::size_t pixelOffset = static_cast<std::size_t>(pixel) * channelCount;
        for (std::size_t channel = 0; channel < channelCount; ++channel) {
            const auto error = absoluteDifference(
                actual.pixels[pixelOffset + channel],
                expected.pixels[pixelOffset + channel]
            );
            totalChannelError += error;
            result.maximumPixelError = std::max(result.maximumPixelError, error);
            outsideTolerance = outsideTolerance || error > tolerance.channelError;
        }
        if (outsideTolerance) {
            ++result.pixelsOutsideTolerance;
        }
    }

    result.meanPixelError = static_cast<double>(totalChannelError) /
        static_cast<double>(result.totalPixels * channelCount);
    result.pixelsOutsidePercentage = 100.0 *
        static_cast<double>(result.pixelsOutsideTolerance) /
        static_cast<double>(result.totalPixels);
    result.passed =
        result.meanPixelError <= tolerance.meanError &&
        result.maximumPixelError <= tolerance.maximumError &&
        result.pixelsOutsidePercentage <= tolerance.pixelsOutsidePercentage;
    return result;
}

ComparisonResult compareAgainstGolden(
    const Image& actual,
    const std::filesystem::path& goldenPath,
    const std::filesystem::path& outputDirectory,
    const ComparisonTolerance& tolerance
) {
    const Image expected = loadPng(goldenPath);
    const ComparisonResult result = compareImages(actual, expected, tolerance);
    writePng(outputDirectory / "actual.png", actual);
    if (!result.passed) {
        writePng(outputDirectory / "expected.png", expected);
        writePng(outputDirectory / "difference.png", differenceImage(actual, expected));
    }
    return result;
}

} // namespace vshade::tests::visual
