#include "scene/SceneAudioSystem.hpp"

#include "asset/AssetManager.hpp"
#include "audio/AudioClip.hpp"
#include "audio/AudioService.hpp"
#include "audio/AudioVoice.hpp"
#include "core/EngineServices.hpp"
#include "core/Log.hpp"
#include "scene/Components.hpp"
#include "scene/Entity.hpp"
#include "scene/Scene.hpp"

#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace vshade::scene {

struct SceneAudioSystem::Impl {
    struct ActiveSource {
        audio::AudioClipHandle clip;
        audio::AudioVoice voice;
    };

    explicit Impl(core::EngineServices& engineServices)
        : services(engineServices) {}

    [[nodiscard]] std::shared_ptr<audio::AudioClip> resolveClip(
        const AudioSourceComponent& source
    ) {
        if (source.clipAsset.valid()) {
            return services.assets().loadResource(source.clipAsset).shared();
        }
        if (source.clip.valid()) {
            if (auto loaded = services.assets().get(source.clip)) {
                return loaded;
            }
            throw std::invalid_argument(
                "AudioSourceComponent has only an unresolved legacy clip handle"
            );
        }
        throw std::invalid_argument("AudioSourceComponent has no clip reference");
    }

    [[nodiscard]] audio::AudioClipHandle sourceHandle(
        const AudioSourceComponent& source
    ) const noexcept {
        return source.clipAsset.valid() ? source.clipAsset.handle() : source.clip;
    }

    void synchronizeSources() {
        for (auto iterator = voices.begin(); iterator != voices.end();) {
            const Entity entity = scene->findEntity(iterator->first);
            if (!entity || !entity.hasComponents<AudioSourceComponent>()) {
                iterator->second.voice.stop();
                iterator = voices.erase(iterator);
                continue;
            }
            const auto& source = entity.component<AudioSourceComponent>();
            if (!source.playOnStart || sourceHandle(source) != iterator->second.clip) {
                iterator->second.voice.stop();
                iterator = voices.erase(iterator);
                continue;
            }
            iterator->second.voice.setVolume(source.volume);
            iterator->second.voice.setPitch(source.pitch);
            if (source.spatial) {
                iterator->second.voice.setPosition(
                    entity.component<TransformComponent>().transform.position()
                );
            }
            ++iterator;
        }

        const auto sources = scene->view<
            const UUIDComponent,
            const TransformComponent,
            const AudioSourceComponent
        >();
        for (const auto [handle, uuid, transform, source] : sources.each()) {
            (void)handle;
            if (!source.playOnStart || voices.contains(uuid.uuid)) {
                continue;
            }
            const std::shared_ptr<audio::AudioClip> clip = resolveClip(source);
            audio::AudioVoice voice = services.audio().play(clip, {
                .bus = source.bus,
                .loadMode = source.loadMode,
                .volume = source.volume,
                .pitch = source.pitch,
                .looping = source.looping,
                .spatial = source.spatial,
                .position = transform.transform.position(),
                .attenuation = source.attenuation,
                .minimumDistance = source.minimumDistance,
                .maximumDistance = source.maximumDistance,
            });
            voices.emplace(
                uuid.uuid,
                ActiveSource{.clip = sourceHandle(source), .voice = std::move(voice)}
            );
        }
    }

    void synchronizeListener() {
        const TransformComponent* listenerTransform = nullptr;
        const auto listeners = scene->view<
            const TransformComponent,
            const AudioListenerComponent
        >();
        for (const auto [handle, transform, listener] : listeners.each()) {
            (void)handle;
            if (!listener.active) {
                continue;
            }
            if (listenerTransform != nullptr) {
                throw std::logic_error("A scene can have only one active audio listener");
            }
            listenerTransform = &transform;
        }
        if (listenerTransform != nullptr) {
            const math::Transform& transform = listenerTransform->transform;
            services.audio().setListenerTransform(
                transform.position(),
                transform.forward(),
                transform.up()
            );
        }
    }

    core::EngineServices& services;
    Scene* scene = nullptr;
    std::unordered_map<std::uint64_t, ActiveSource> voices;
};

SceneAudioSystem::SceneAudioSystem(core::EngineServices& services)
    : m_impl(std::make_unique<Impl>(services)) {}

SceneAudioSystem::~SceneAudioSystem() { detachScene(); }
SceneAudioSystem::SceneAudioSystem(SceneAudioSystem&&) noexcept = default;

SceneAudioSystem& SceneAudioSystem::operator=(SceneAudioSystem&& other) noexcept {
    if (this != &other) {
        detachScene();
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

void SceneAudioSystem::attachScene(Scene& scene) {
    if (m_impl->scene != nullptr) {
        throw std::logic_error("SceneAudioSystem already has an attached scene");
    }
    m_impl->scene = &scene;
    try {
        m_impl->synchronizeListener();
        m_impl->synchronizeSources();
    } catch (...) {
        detachScene();
        throw;
    }
}

void SceneAudioSystem::update() {
    if (m_impl->scene == nullptr) {
        throw std::logic_error("SceneAudioSystem requires an attached scene");
    }
    m_impl->synchronizeListener();
    m_impl->synchronizeSources();
}

void SceneAudioSystem::detachScene() noexcept {
    if (!m_impl) {
        return;
    }
    for (auto& [uuid, source] : m_impl->voices) {
        (void)uuid;
        source.voice.stop();
    }
    m_impl->voices.clear();
    m_impl->scene = nullptr;
}

bool SceneAudioSystem::hasAttachedScene() const noexcept {
    return m_impl && m_impl->scene != nullptr;
}

std::size_t SceneAudioSystem::voiceCount() const noexcept {
    return m_impl ? m_impl->voices.size() : 0;
}

} // namespace vshade::scene
