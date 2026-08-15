#include "core/EngineServices.hpp"

#include "asset/AssetManager.hpp"
#include "audio/AudioEngine.hpp"
#include "audio/AudioService.hpp"
#include "script/NativeScriptRegistry.hpp"
#include "core/TypeRegistry.hpp"

#include <utility>

namespace vshade::core {

struct EngineServices::Impl {
    explicit Impl(const audio::AudioEngineConfig& config)
        : audioService(audioEngine, assets), types(scripts), audioConfig(config) {}

    asset::AssetManager assets;
    audio::AudioEngine audioEngine;
    audio::AudioService audioService;
    script::NativeScriptRegistry scripts;
    TypeRegistry types;
    audio::AudioEngineConfig audioConfig;
    bool audioInitialized = false;
};

EngineServices::EngineServices(const audio::AudioEngineConfig& audioConfig)
    : m_impl(std::make_unique<Impl>(audioConfig)) {}

EngineServices::~EngineServices() {
    shutdown();
}

EngineServices::EngineServices(EngineServices&&) noexcept = default;

EngineServices& EngineServices::operator=(EngineServices&& other) noexcept {
    if (this != &other) {
        shutdown();
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

asset::AssetManager& EngineServices::assets() noexcept {
    return m_impl->assets;
}

const asset::AssetManager& EngineServices::assets() const noexcept {
    return m_impl->assets;
}

audio::AudioService& EngineServices::audio() {
    if (!m_impl->audioInitialized) {
        m_impl->audioEngine.initialize(m_impl->audioConfig);
        m_impl->audioInitialized = true;
    }
    return m_impl->audioService;
}

audio::AudioEngine& EngineServices::audioEngine() {
    static_cast<void>(audio());
    return m_impl->audioEngine;
}

script::NativeScriptRegistry& EngineServices::scripts() noexcept {
    return m_impl->scripts;
}

const script::NativeScriptRegistry& EngineServices::scripts() const noexcept {
    return m_impl->scripts;
}

TypeRegistry& EngineServices::types() noexcept { return m_impl->types; }

void EngineServices::shutdown() noexcept {
    if (!m_impl) {
        return;
    }
    if (m_impl->audioInitialized) {
        m_impl->audioService.stopMusic();
        m_impl->audioEngine.shutdown();
        m_impl->audioInitialized = false;
    }
    m_impl->assets.clear();
}

} // namespace vshade::core
