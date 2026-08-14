#include "scene/SceneSerializer.hpp"

#include "math/quaternion.hpp"
#include "scene/Components.hpp"
#include "scene/Scene.hpp"
#include "scene/SceneComponentRegistry.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace vshade::scene {

SceneSerializer::SceneSerializer(Scene& scene) noexcept
    : m_scene(scene) {}

const std::string& SceneSerializer::lastError() const noexcept {
    return m_lastError;
}

void SceneSerializer::setError(std::string error) const {
    m_lastError = std::move(error);
}

namespace {

using Json = nlohmann::json;

constexpr int sceneFormatVersion = 2;

[[nodiscard]] double serializedFloat(const float value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument("scene components must contain finite numbers");
    }
    return value == 0.0F ? 0.0 : static_cast<double>(value);
}

[[nodiscard]] Json vector2(const math::Vec2& value) {
    return Json::array({serializedFloat(value.x), serializedFloat(value.y)});
}

[[nodiscard]] Json vector3(const math::Vec3& value) {
    return Json::array({
        serializedFloat(value.x),
        serializedFloat(value.y),
        serializedFloat(value.z),
    });
}

[[nodiscard]] Json vector4(const math::Vec4& value) {
    return Json::array({
        serializedFloat(value.x),
        serializedFloat(value.y),
        serializedFloat(value.z),
        serializedFloat(value.w),
    });
}

template<std::size_t Size>
[[nodiscard]] std::array<float, Size> floatArray(const Json& json) {
    if (!json.is_array() || json.size() != Size) {
        throw std::invalid_argument("expected fixed-size float array");
    }

    std::array<float, Size> result{};
    for (std::size_t index = 0; index < Size; ++index) {
        result[index] = json.at(index).get<float>();
        if (!std::isfinite(result[index])) {
            throw std::invalid_argument("scene components must contain finite numbers");
        }
    }
    return result;
}

[[nodiscard]] std::uint64_t parseUuid(const Json& json) {
    const std::string text = json.get<std::string>();
    std::size_t parsed = 0;
    const unsigned long long value = std::stoull(text, &parsed, 10);
    if (parsed != text.size() || value == 0) {
        throw std::invalid_argument("invalid entity UUID");
    }
    return static_cast<std::uint64_t>(value);
}

} // namespace

bool SceneSerializer::serialize(
    const std::filesystem::path& path,
    const SceneJsonFormat format
) const {
    m_lastError.clear();
    try {
        Json root{
            {"FormatVersion", sceneFormatVersion},
            {"Scene", m_scene.m_name},
            {"Entities", Json::array()},
        };

        const entt::registry& registry = m_scene.m_registry;
        const auto* entityStorage = registry.storage<entt::entity>();
        std::vector<entt::entity> orderedEntities;
        orderedEntities.reserve(entityStorage->size());
        for (const auto [handle] : entityStorage->each()) {
            if (!registry.all_of<UUIDComponent, TagComponent, TransformComponent>(handle)) {
                throw std::logic_error(
                    "every scene entity requires UUID, tag, and transform components"
                );
            }
            orderedEntities.push_back(handle);
        }
        std::sort(
            orderedEntities.begin(),
            orderedEntities.end(),
            [&registry](const entt::entity left, const entt::entity right) {
                return registry.get<UUIDComponent>(left).uuid <
                    registry.get<UUIDComponent>(right).uuid;
            }
        );

        const auto entities = registry.view<
            UUIDComponent,
            TagComponent,
            TransformComponent
        >();
        for (const entt::entity handle : orderedEntities) {
            const auto& uuid = entities.get<UUIDComponent>(handle);
            const auto& tag = entities.get<TagComponent>(handle);
            const auto& transform = entities.get<TransformComponent>(handle).transform;

            Json entity{
                {"Entity", std::to_string(uuid.uuid)},
                {"Tag", tag.tag},
                {"Transform", {
                    {"Position", vector3(transform.position())},
                    {"Rotation", vector4({
                        transform.rotation().x,
                        transform.rotation().y,
                        transform.rotation().z,
                        transform.rotation().w,
                    })},
                    {"Scale", vector3(transform.scale())},
                }},
            };

            if (registry.all_of<SpriteRendererComponent>(handle)) {
                const auto& sprite = registry.get<SpriteRendererComponent>(handle);
                entity["SpriteRenderer"] = {
                    {"Texture", sprite.texturePath.generic_string()},
                    {"Color", vector4(sprite.color)},
                    {"Tiling", vector2(sprite.tiling)},
                    {"SortingLayer", sprite.sortingLayer},
                };
            }
            for (const auto& handler : SceneComponentRegistry::handlers()) {
                if (handler.has(registry, handle)) {
                    entity["Components"][handler.name] =
                        handler.serialize(registry, handle);
                }
            }
            root["Entities"].push_back(std::move(entity));
        }

        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("failed to open scene file for writing: " + path.string());
        }
        if (format == SceneJsonFormat::Compact) {
            output << root.dump();
        } else {
            output << root.dump(2) << '\n';
        }
        if (!output) {
            throw std::runtime_error("failed to write scene file: " + path.string());
        }
        return true;
    } catch (const std::exception& error) {
        setError(error.what());
        return false;
    } catch (...) {
        setError("unknown scene serialization error");
        return false;
    }
}

bool SceneSerializer::deserialize(const std::filesystem::path& path) {
    m_lastError.clear();
    try {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("failed to open scene file: " + path.string());
        }

        Json root;
        input >> root;
        const int loadedFormatVersion = root.at("FormatVersion").get<int>();
        if ((loadedFormatVersion != 1 && loadedFormatVersion != sceneFormatVersion) ||
            !root.at("Entities").is_array()) {
            throw std::invalid_argument("unsupported scene format version or entity list");
        }
        const std::string loadedSceneName = root.at("Scene").get<std::string>();

        entt::registry loadedRegistry;
        std::unordered_set<std::uint64_t> loadedUuids;
        std::unordered_map<std::uint64_t, entt::entity> loadedEntitiesByUuid;
        for (const Json& serializedEntity : root.at("Entities")) {
            const std::uint64_t uuid = parseUuid(serializedEntity.at("Entity"));
            if (!loadedUuids.insert(uuid).second) {
                throw std::invalid_argument("duplicate entity UUID in scene file");
            }

            const Json& serializedTransform = serializedEntity.at("Transform");
            const auto position = floatArray<3>(serializedTransform.at("Position"));
            const auto scale = floatArray<3>(serializedTransform.at("Scale"));
            math::Quat rotation;
            if (loadedFormatVersion == 1) {
                const auto euler = floatArray<3>(serializedTransform.at("Rotation"));
                rotation = math::fromEuler({euler[0], euler[1], euler[2]});
            } else {
                const auto quaternion = floatArray<4>(serializedTransform.at("Rotation"));
                rotation = math::Quat{
                    quaternion[3],
                    quaternion[0],
                    quaternion[1],
                    quaternion[2]
                };
            }

            const entt::entity handle = loadedRegistry.create();
            loadedEntitiesByUuid.emplace(uuid, handle);
            loadedRegistry.emplace<UUIDComponent>(handle, uuid);
            loadedRegistry.emplace<TagComponent>(
                handle,
                serializedEntity.at("Tag").get<std::string>()
            );
            loadedRegistry.emplace<TransformComponent>(
                handle,
                math::Transform(
                    {position[0], position[1], position[2]},
                    rotation,
                    {scale[0], scale[1], scale[2]}
                )
            );

            if (const auto sprite = serializedEntity.find("SpriteRenderer");
                sprite != serializedEntity.end()) {
                const auto color = floatArray<4>(sprite->at("Color"));
                const auto tiling = floatArray<2>(sprite->at("Tiling"));
                loadedRegistry.emplace<SpriteRendererComponent>(
                    handle,
                    std::filesystem::path(sprite->at("Texture").get<std::string>()),
                    math::Vec4{color[0], color[1], color[2], color[3]},
                    math::Vec2{tiling[0], tiling[1]},
                    sprite->at("SortingLayer").get<std::int32_t>()
                );
            }

            if (const auto components = serializedEntity.find("Components");
                components != serializedEntity.end()) {
                if (!components->is_object()) {
                    throw std::invalid_argument("Components must be a JSON object");
                }
                for (const auto& [name, value] : components->items()) {
                    const auto& handlers = SceneComponentRegistry::handlers();
                    const auto handler = std::ranges::find_if(
                        handlers,
                        [&name](const auto& candidate) { return candidate.name == name; }
                    );
                    if (handler == handlers.end()) {
                        throw std::invalid_argument(
                            "No serializer is registered for component: " + name
                        );
                    }
                    handler->deserialize(loadedRegistry, handle, value);
                }
            }
        }

        m_scene.m_registry = std::move(loadedRegistry);
        m_scene.m_entitiesByUuid = std::move(loadedEntitiesByUuid);
        m_scene.m_name = loadedSceneName;
        ++m_scene.m_generation;
        if (m_scene.m_generation == 0) {
            ++m_scene.m_generation;
        }
        return true;
    } catch (const std::exception& error) {
        setError(error.what());
        return false;
    } catch (...) {
        setError("unknown scene deserialization error");
        return false;
    }
}

} // namespace vshade::scene
