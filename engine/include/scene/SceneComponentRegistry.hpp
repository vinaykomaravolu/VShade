#pragma once

#include <entt/entity/registry.hpp>
#include <entt/core/type_info.hpp>
#include <nlohmann/json.hpp>
#include <rfl/json.hpp>

#include <concepts>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vshade::scene {

/**
 * @brief Registers user components that participate in scene duplication and JSON files.
 *
 * Registration is normally performed once during application startup, before scenes are
 * loaded or duplicated. Component names are persisted and therefore form part of the
 * scene-file format.
 */
class SceneComponentRegistry final {
public:
    using Json = nlohmann::json;

    /**
     * @brief Registers an aggregate component using its unqualified C++ type name.
     *
     * reflect-cpp discovers the component's public fields automatically. Prefer
     * the named overload for long-lived scene formats because renaming the C++
     * type also changes this inferred serialized name.
     */
    template<typename Component>
        requires std::copy_constructible<Component>
    static void registerComponent() {
        std::string name{entt::type_name<Component>::value()};
        if (const std::size_t separator = name.rfind("::");
            separator != std::string::npos) {
            name.erase(0, separator + 2);
        }
        registerComponent<Component>(std::move(name));
    }

    /**
     * @brief Registers an aggregate component with automatic field serialization.
     * @param name Stable component name written to scene files.
     */
    template<typename Component>
        requires std::copy_constructible<Component>
    static void registerComponent(std::string name) {
        registerComponent<Component>(
            std::move(name),
            [](const Component& component) {
                return Json::parse(rfl::json::write(component));
            },
            [](const Json& json) {
                auto result = rfl::json::read<Component>(json.dump());
                if (!result.has_value()) {
                    throw std::invalid_argument(
                        "Failed to deserialize reflected component: " +
                        result.error().what()
                    );
                }
                return std::move(result).value();
            }
        );
    }

    /** @brief Registers custom conversion callbacks for a component. */
    template<typename Component, typename Serialize, typename Deserialize>
        requires std::copy_constructible<Component>
    static void registerComponent(
        std::string name,
        Serialize serialize,
        Deserialize deserialize
    ) {
        Handler handler;
        handler.name = std::move(name);
        handler.type = entt::type_hash<Component>::value();
        handler.has = [](const entt::registry& registry, const entt::entity entity) {
            return registry.all_of<Component>(entity);
        };
        handler.serialize = [serialize = std::move(serialize)](
            const entt::registry& registry,
            const entt::entity entity
        ) {
            return Json(serialize(registry.get<Component>(entity)));
        };
        handler.deserialize = [deserialize = std::move(deserialize)](
            entt::registry& registry,
            const entt::entity entity,
            const Json& json
        ) {
            registry.emplace<Component>(entity, deserialize(json));
        };
        handler.clone = [](const entt::registry& sourceRegistry,
                           const entt::entity source,
                           entt::registry& destinationRegistry,
                           const entt::entity destination) {
            if (sourceRegistry.all_of<Component>(source)) {
                Component copy = sourceRegistry.get<Component>(source);
                destinationRegistry.emplace<Component>(
                    destination,
                    std::move(copy)
                );
            }
        };
        add(std::move(handler));
    }

private:
    struct Handler {
        std::string name;
        entt::id_type type{};
        std::function<bool(const entt::registry&, entt::entity)> has;
        std::function<Json(const entt::registry&, entt::entity)> serialize;
        std::function<void(entt::registry&, entt::entity, const Json&)> deserialize;
        std::function<void(
            const entt::registry&,
            entt::entity,
            entt::registry&,
            entt::entity
        )> clone;
    };

    friend class Scene;
    friend class Prefab;
    friend class SceneSerializer;

    static void add(Handler handler);
    [[nodiscard]] static std::vector<Handler>& mutableHandlers();
    [[nodiscard]] static const std::vector<Handler>& handlers();
};

} // namespace vshade::scene
