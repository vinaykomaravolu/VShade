#include "asset/loader/AudioLoader.hpp"

#include "audio/AudioClip.hpp"

#include <miniaudio.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace vshade::asset {

std::shared_ptr<audio::AudioClip> AudioLoader::load(
    const std::filesystem::path& path
) const {
    const std::filesystem::path normalizedPath = path.lexically_normal();
    if (normalizedPath.empty()) {
        throw std::invalid_argument("An audio clip path cannot be empty");
    }

    ma_decoder decoder{};
    const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
#if defined(_WIN32)
    ma_result result = ma_decoder_init_file_w(normalizedPath.c_str(), &config, &decoder);
#else
    const std::string nativePath = normalizedPath.string();
    ma_result result = ma_decoder_init_file(nativePath.c_str(), &config, &decoder);
#endif
    if (result != MA_SUCCESS) {
        throw std::runtime_error(
            "Failed to decode audio clip " + normalizedPath.string() + ": " +
            ma_result_description(result)
        );
    }

    struct DecoderGuard final {
        ma_decoder* decoder;
        ~DecoderGuard() { ma_decoder_uninit(decoder); }
    } guard{&decoder};

    if (decoder.outputChannels == 0 || decoder.outputSampleRate == 0) {
        throw std::runtime_error("Audio clip reports an invalid format");
    }

    ma_uint64 frameCount = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount) != MA_SUCCESS) {
        throw std::runtime_error("Audio clip duration could not be read");
    }

    return std::shared_ptr<audio::AudioClip>(new audio::AudioClip(
        normalizedPath,
        decoder.outputChannels,
        decoder.outputSampleRate,
        frameCount
    ));
}

} // namespace vshade::asset
