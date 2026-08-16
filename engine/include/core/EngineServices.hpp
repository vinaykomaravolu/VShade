#pragma once

#include "audio/AudioTypes.hpp"

#include <memory>

namespace vshade::asset {
class AssetManager;
}

namespace vshade::audio {
class AudioEngine;
class AudioService;
}

namespace vshade::script {
class NativeScriptRegistry;
}

namespace vshade::core {

class TypeRegistry;

/** @brief Application-lifetime owner of shared engine services. */
class EngineServices final {
public:
    explicit EngineServices(const audio::AudioEngineConfig& audioConfig = {});
    ~EngineServices();

    EngineServices(const EngineServices&) = delete;
    EngineServices& operator=(const EngineServices&) = delete;
    EngineServices(EngineServices&&) noexcept;
    EngineServices& operator=(EngineServices&&) noexcept;

    [[nodiscard]] asset::AssetManager& assets() noexcept;
    [[nodiscard]] const asset::AssetManager& assets() const noexcept;

    /** @brief Returns the lazily initialized application audio engine. */
    [[nodiscard]] audio::AudioService& audio();

    /** @brief Returns the lower-level audio engine used by the facade. */
    [[nodiscard]] audio::AudioEngine& audioEngine();

    [[nodiscard]] script::NativeScriptRegistry& scripts() noexcept;
    [[nodiscard]] const script::NativeScriptRegistry& scripts() const noexcept;

    /** @brief Unified startup registration for persistent game types. */
    [[nodiscard]] TypeRegistry& types() noexcept;

    /** @brief Releases initialized services; safe to call repeatedly. */
    void shutdown() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::core
