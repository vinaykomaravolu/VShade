#include "scene/SceneSerializer.hpp"

#include "math/Quaternion.hpp"
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
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace vshade::scene {

SceneSerializer::SceneSerializer(Scene& scene) noexcept
    : m_scene(scene),
      m_writableScene(&scene) {}

SceneSerializer::SceneSerializer(const Scene& scene) noexcept
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

[[nodiscard]] float finiteFloat(const Json& json) {
    const float value = json.get<float>();
    if (!std::isfinite(value)) {
        throw std::invalid_argument("scene components must contain finite numbers");
    }
    return value;
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

[[nodiscard]] asset::AssetId parseAssetId(const Json& json) {
    const std::string text = json.get<std::string>();
    std::size_t parsed = 0;
    const unsigned long long value = std::stoull(text, &parsed, 10);
    if (parsed != text.size()) {
        throw std::invalid_argument("invalid asset identifier");
    }
    return static_cast<asset::AssetId>(value);
}

template<typename Resource>
[[nodiscard]] Json serializeAssetReference(
    const asset::AssetReference<Resource>& reference
) {
    if (!reference.valid()) {
        throw std::invalid_argument("asset references must contain an ID and source path");
    }
    return {
        {"Id", std::to_string(reference.handle().id())},
        {"Path", reference.sourcePath().generic_string()},
    };
}

template<typename Resource>
[[nodiscard]] asset::AssetReference<Resource> deserializeAssetReference(
    const Json& json
) {
    const asset::AssetId id = parseAssetId(json.at("Id"));
    const std::filesystem::path path = json.at("Path").get<std::string>();
    if (id == asset::invalidAssetId || path.empty()) {
        throw std::invalid_argument("asset references must contain an ID and source path");
    }
    return {asset::AssetHandle<Resource>::fromId(id), path};
}

[[nodiscard]] std::string_view audioBusName(const audio::AudioBus bus) {
    switch (bus) {
        case audio::AudioBus::Master: return "Master";
        case audio::AudioBus::Music: return "Music";
        case audio::AudioBus::SFX: return "SFX";
    }
    throw std::invalid_argument("unknown audio bus");
}

[[nodiscard]] audio::AudioBus parseAudioBus(const Json& json) {
    const std::string value = json.get<std::string>();
    if (value == "Master") return audio::AudioBus::Master;
    if (value == "Music") return audio::AudioBus::Music;
    if (value == "SFX") return audio::AudioBus::SFX;
    throw std::invalid_argument("unknown audio bus: " + value);
}

[[nodiscard]] std::string_view audioLoadModeName(const audio::AudioLoadMode mode) {
    switch (mode) {
        case audio::AudioLoadMode::Decode: return "Decode";
        case audio::AudioLoadMode::Stream: return "Stream";
    }
    throw std::invalid_argument("unknown audio load mode");
}

[[nodiscard]] audio::AudioLoadMode parseAudioLoadMode(const Json& json) {
    const std::string value = json.get<std::string>();
    if (value == "Decode") return audio::AudioLoadMode::Decode;
    if (value == "Stream") return audio::AudioLoadMode::Stream;
    throw std::invalid_argument("unknown audio load mode: " + value);
}

[[nodiscard]] std::string_view attenuationName(
    const audio::AttenuationModel attenuation
) {
    switch (attenuation) {
        case audio::AttenuationModel::None: return "None";
        case audio::AttenuationModel::Inverse: return "Inverse";
        case audio::AttenuationModel::Linear: return "Linear";
        case audio::AttenuationModel::Exponential: return "Exponential";
    }
    throw std::invalid_argument("unknown audio attenuation model");
}

[[nodiscard]] audio::AttenuationModel parseAttenuation(const Json& json) {
    const std::string value = json.get<std::string>();
    if (value == "None") return audio::AttenuationModel::None;
    if (value == "Inverse") return audio::AttenuationModel::Inverse;
    if (value == "Linear") return audio::AttenuationModel::Linear;
    if (value == "Exponential") return audio::AttenuationModel::Exponential;
    throw std::invalid_argument("unknown audio attenuation model: " + value);
}

[[nodiscard]] std::string_view scriptBackendName(const script::ScriptBackend backend) {
    switch (backend) {
        case script::ScriptBackend::NativeCpp: return "NativeCpp";
        case script::ScriptBackend::CSharp: return "CSharp";
        case script::ScriptBackend::Lua: return "Lua";
        case script::ScriptBackend::Custom: return "Custom";
    }
    throw std::invalid_argument("unknown script backend");
}

[[nodiscard]] script::ScriptBackend parseScriptBackend(const Json& json) {
    const std::string value = json.get<std::string>();
    if (value == "NativeCpp") return script::ScriptBackend::NativeCpp;
    if (value == "CSharp") return script::ScriptBackend::CSharp;
    if (value == "Lua") return script::ScriptBackend::Lua;
    if (value == "Custom") return script::ScriptBackend::Custom;
    throw std::invalid_argument("unknown script backend: " + value);
}

[[nodiscard]] Json serializeScripts(const ScriptComponent& component) {
    Json scripts = Json::array();
    for (const ScriptBinding& binding : component.scripts) {
        if (binding.typeName.empty()) {
            throw std::invalid_argument("script type name cannot be empty");
        }
        scripts.push_back({
            {"Backend", scriptBackendName(binding.backend)},
            {"Type", binding.typeName},
            {"Enabled", binding.enabled},
        });
    }
    return scripts;
}

[[nodiscard]] ScriptComponent deserializeScripts(const Json& json) {
    if (!json.is_array()) {
        throw std::invalid_argument("Scripts must be a JSON array");
    }
    ScriptComponent component;
    component.scripts.reserve(json.size());
    for (const Json& value : json) {
        ScriptBinding binding{
            .backend = parseScriptBackend(value.at("Backend")),
            .typeName = value.at("Type").get<std::string>(),
            .enabled = value.at("Enabled").get<bool>(),
        };
        if (binding.typeName.empty()) {
            throw std::invalid_argument("script type name cannot be empty");
        }
        component.scripts.push_back(std::move(binding));
    }
    return component;
}

[[nodiscard]] Json serializeLight(const LightComponent& component) {
    Json json{{"Enabled", component.enabled}};
    std::visit(
        [&json](const auto& light) {
            using LightType = std::decay_t<decltype(light)>;
            renderer::Lighting validator;
            json["Color"] = vector3(light.color);
            json["Intensity"] = serializedFloat(light.intensity);
            if constexpr (std::is_same_v<LightType, renderer::DirectionalLight>) {
                validator.addDirectionalLight(light);
                json["Type"] = "Directional";
                json["Direction"] = vector3(light.direction);
            } else {
                validator.addPointLight(light);
                json["Type"] = "Point";
                json["Position"] = vector3(light.position);
                json["Range"] = serializedFloat(light.range);
            }
        },
        component.light
    );
    return json;
}

[[nodiscard]] LightComponent deserializeLight(const Json& json) {
    const std::string type = json.at("Type").get<std::string>();
    const auto color = floatArray<3>(json.at("Color"));
    const float intensity = json.at("Intensity").get<float>();
    LightComponent component{.enabled = json.at("Enabled").get<bool>()};
    renderer::Lighting validator;

    if (type == "Directional") {
        const auto direction = floatArray<3>(json.at("Direction"));
        const renderer::DirectionalLight light{
            .direction = {direction[0], direction[1], direction[2]},
            .color = {color[0], color[1], color[2]},
            .intensity = intensity,
        };
        validator.addDirectionalLight(light);
        component.light = validator.directionalLights().back();
    } else if (type == "Point") {
        const auto position = floatArray<3>(json.at("Position"));
        const renderer::PointLight light{
            .position = {position[0], position[1], position[2]},
            .color = {color[0], color[1], color[2]},
            .intensity = intensity,
            .range = json.at("Range").get<float>(),
        };
        validator.addPointLight(light);
        component.light = light;
    } else {
        throw std::invalid_argument("unknown light type: " + type);
    }
    return component;
}

[[nodiscard]] std::string_view bodyTypeName(const physics::BodyType type) {
    switch (type) {
        case physics::BodyType::Static: return "Static";
        case physics::BodyType::Dynamic: return "Dynamic";
        case physics::BodyType::Kinematic: return "Kinematic";
    }
    throw std::invalid_argument("unknown physics body type");
}

[[nodiscard]] physics::BodyType parseBodyType(const Json& json) {
    const std::string value = json.get<std::string>();
    if (value == "Static") return physics::BodyType::Static;
    if (value == "Dynamic") return physics::BodyType::Dynamic;
    if (value == "Kinematic") return physics::BodyType::Kinematic;
    throw std::invalid_argument("unknown physics body type: " + value);
}

[[nodiscard]] Json serializeCollisionFilter(const physics::CollisionFilter& filter) {
    return {{"Layer", filter.layer}, {"Mask", filter.mask}};
}

[[nodiscard]] physics::CollisionFilter deserializeCollisionFilter(const Json& json) {
    return {
        .layer = json.at("Layer").get<std::uint32_t>(),
        .mask = json.at("Mask").get<std::uint32_t>(),
    };
}

[[nodiscard]] Json serializeRigidBody2D(const RigidBody2DComponent& component) {
    const auto& settings = component.settings;
    return {
        {"Type", bodyTypeName(settings.type)},
        {"LinearVelocity", vector2(settings.linearVelocity)},
        {"AngularVelocity", serializedFloat(settings.angularVelocity)},
        {"LinearDamping", serializedFloat(settings.linearDamping)},
        {"AngularDamping", serializedFloat(settings.angularDamping)},
        {"GravityScale", serializedFloat(settings.gravityScale)},
        {"FixedRotation", settings.fixedRotation},
        {"ContinuousCollision", settings.continuousCollision},
        {"Enabled", settings.enabled},
        {"Collision", serializeCollisionFilter(settings.collision)},
    };
}

[[nodiscard]] RigidBody2DComponent deserializeRigidBody2D(const Json& json) {
    const auto velocity = floatArray<2>(json.at("LinearVelocity"));
    physics::PhysicsBody2DSettings settings;
    settings.type = parseBodyType(json.at("Type"));
    settings.linearVelocity = {velocity[0], velocity[1]};
    settings.angularVelocity = finiteFloat(json.at("AngularVelocity"));
    settings.linearDamping = finiteFloat(json.at("LinearDamping"));
    settings.angularDamping = finiteFloat(json.at("AngularDamping"));
    settings.gravityScale = finiteFloat(json.at("GravityScale"));
    settings.fixedRotation = json.at("FixedRotation").get<bool>();
    settings.continuousCollision = json.at("ContinuousCollision").get<bool>();
    settings.enabled = json.at("Enabled").get<bool>();
    settings.collision = deserializeCollisionFilter(json.at("Collision"));
    if (settings.linearDamping < 0.0F || settings.angularDamping < 0.0F) {
        throw std::invalid_argument("2D rigid-body damping must be non-negative");
    }
    return {.settings = settings};
}

[[nodiscard]] Json serializeShape2D(const physics::PhysicsShape2D& shape) {
    return std::visit([](const auto& typedShape) -> Json {
        using Shape = std::decay_t<decltype(typedShape)>;
        if constexpr (std::is_same_v<Shape, physics::BoxShape2D>) {
            return {{"Type", "Box"}, {"HalfExtents", vector2(typedShape.halfExtents)}};
        } else if constexpr (std::is_same_v<Shape, physics::CircleShape2D>) {
            return {{"Type", "Circle"}, {"Radius", serializedFloat(typedShape.radius)}};
        } else {
            return {
                {"Type", "Capsule"},
                {"HalfHeight", serializedFloat(typedShape.halfHeight)},
                {"Radius", serializedFloat(typedShape.radius)},
            };
        }
    }, shape);
}

[[nodiscard]] physics::PhysicsShape2D deserializeShape2D(const Json& json) {
    const std::string type = json.at("Type").get<std::string>();
    if (type == "Box") {
        const auto value = floatArray<2>(json.at("HalfExtents"));
        if (value[0] <= 0.0F || value[1] <= 0.0F) {
            throw std::invalid_argument("2D box half extents must be positive");
        }
        return physics::BoxShape2D{{value[0], value[1]}};
    }
    if (type == "Circle") {
        const float radius = finiteFloat(json.at("Radius"));
        if (radius <= 0.0F) {
            throw std::invalid_argument("2D circle radius must be positive");
        }
        return physics::CircleShape2D{radius};
    }
    if (type == "Capsule") {
        const float halfHeight = finiteFloat(json.at("HalfHeight"));
        const float radius = finiteFloat(json.at("Radius"));
        if (halfHeight < 0.0F || radius <= 0.0F) {
            throw std::invalid_argument("2D capsule dimensions are invalid");
        }
        return physics::CapsuleShape2D{halfHeight, radius};
    }
    throw std::invalid_argument("unknown 2D physics shape: " + type);
}

[[nodiscard]] Json serializeCollider2D(const Collider2DComponent& component) {
    return {
        {"Shape", serializeShape2D(component.shape)},
        {"Material", {
            {"Density", serializedFloat(component.material.density)},
            {"Friction", serializedFloat(component.material.friction)},
            {"Restitution", serializedFloat(component.material.restitution)},
        }},
        {"Offset", vector2(component.offset)},
        {"Sensor", component.sensor},
    };
}

[[nodiscard]] Collider2DComponent deserializeCollider2D(const Json& json) {
    const auto offset = floatArray<2>(json.at("Offset"));
    const Json& material = json.at("Material");
    Collider2DComponent component{
        .shape = deserializeShape2D(json.at("Shape")),
        .material = {
            .density = finiteFloat(material.at("Density")),
            .friction = finiteFloat(material.at("Friction")),
            .restitution = finiteFloat(material.at("Restitution")),
        },
        .offset = {offset[0], offset[1]},
        .sensor = json.at("Sensor").get<bool>(),
    };
    if (component.material.density < 0.0F || component.material.friction < 0.0F ||
        component.material.restitution < 0.0F) {
        throw std::invalid_argument("2D physics material values must be non-negative");
    }
    return component;
}

[[nodiscard]] Json serializeRigidBody3D(const RigidBody3DComponent& component) {
    const auto& settings = component.settings;
    return {
        {"Type", bodyTypeName(settings.type)},
        {"LinearVelocity", vector3(settings.linearVelocity)},
        {"AngularVelocity", vector3(settings.angularVelocity)},
        {"Mass", serializedFloat(settings.mass)},
        {"LinearDamping", serializedFloat(settings.linearDamping)},
        {"AngularDamping", serializedFloat(settings.angularDamping)},
        {"GravityScale", serializedFloat(settings.gravityScale)},
        {"ContinuousCollision", settings.continuousCollision},
        {"Enabled", settings.enabled},
        {"Collision", serializeCollisionFilter(settings.collision)},
    };
}

[[nodiscard]] RigidBody3DComponent deserializeRigidBody3D(const Json& json) {
    const auto linearVelocity = floatArray<3>(json.at("LinearVelocity"));
    const auto angularVelocity = floatArray<3>(json.at("AngularVelocity"));
    physics::PhysicsBody3DSettings settings;
    settings.type = parseBodyType(json.at("Type"));
    settings.linearVelocity = {
        linearVelocity[0], linearVelocity[1], linearVelocity[2]
    };
    settings.angularVelocity = {
        angularVelocity[0], angularVelocity[1], angularVelocity[2]
    };
    settings.mass = finiteFloat(json.at("Mass"));
    settings.linearDamping = finiteFloat(json.at("LinearDamping"));
    settings.angularDamping = finiteFloat(json.at("AngularDamping"));
    settings.gravityScale = finiteFloat(json.at("GravityScale"));
    settings.continuousCollision = json.at("ContinuousCollision").get<bool>();
    settings.enabled = json.at("Enabled").get<bool>();
    settings.collision = deserializeCollisionFilter(json.at("Collision"));
    if (settings.mass <= 0.0F || settings.linearDamping < 0.0F ||
        settings.angularDamping < 0.0F) {
        throw std::invalid_argument("3D rigid-body mass and damping are invalid");
    }
    return {.settings = settings};
}

[[nodiscard]] Json serializeShape3D(const physics::PhysicsShape3D& shape) {
    return std::visit([](const auto& typedShape) -> Json {
        using Shape = std::decay_t<decltype(typedShape)>;
        if constexpr (std::is_same_v<Shape, physics::BoxShape3D>) {
            return {{"Type", "Box"}, {"HalfExtents", vector3(typedShape.halfExtents)}};
        } else if constexpr (std::is_same_v<Shape, physics::SphereShape3D>) {
            return {{"Type", "Sphere"}, {"Radius", serializedFloat(typedShape.radius)}};
        } else {
            return {
                {"Type", "Capsule"},
                {"HalfHeight", serializedFloat(typedShape.halfHeight)},
                {"Radius", serializedFloat(typedShape.radius)},
            };
        }
    }, shape);
}

[[nodiscard]] physics::PhysicsShape3D deserializeShape3D(const Json& json) {
    const std::string type = json.at("Type").get<std::string>();
    if (type == "Box") {
        const auto value = floatArray<3>(json.at("HalfExtents"));
        if (value[0] <= 0.0F || value[1] <= 0.0F || value[2] <= 0.0F) {
            throw std::invalid_argument("3D box half extents must be positive");
        }
        return physics::BoxShape3D{{value[0], value[1], value[2]}};
    }
    if (type == "Sphere") {
        const float radius = finiteFloat(json.at("Radius"));
        if (radius <= 0.0F) {
            throw std::invalid_argument("3D sphere radius must be positive");
        }
        return physics::SphereShape3D{radius};
    }
    if (type == "Capsule") {
        const float halfHeight = finiteFloat(json.at("HalfHeight"));
        const float radius = finiteFloat(json.at("Radius"));
        if (halfHeight <= 0.0F || radius <= 0.0F) {
            throw std::invalid_argument("3D capsule dimensions must be positive");
        }
        return physics::CapsuleShape3D{halfHeight, radius};
    }
    throw std::invalid_argument("unknown 3D physics shape: " + type);
}

[[nodiscard]] Json serializeCollider3D(const Collider3DComponent& component) {
    return {
        {"Shape", serializeShape3D(component.shape)},
        {"Material", {
            {"Friction", serializedFloat(component.material.friction)},
            {"Restitution", serializedFloat(component.material.restitution)},
        }},
        {"Offset", vector3(component.offset)},
        {"Sensor", component.sensor},
    };
}

[[nodiscard]] Collider3DComponent deserializeCollider3D(const Json& json) {
    const auto offset = floatArray<3>(json.at("Offset"));
    const Json& material = json.at("Material");
    Collider3DComponent component{
        .shape = deserializeShape3D(json.at("Shape")),
        .material = {
            .friction = finiteFloat(material.at("Friction")),
            .restitution = finiteFloat(material.at("Restitution")),
        },
        .offset = {offset[0], offset[1], offset[2]},
        .sensor = json.at("Sensor").get<bool>(),
    };
    if (component.material.friction < 0.0F ||
        component.material.restitution < 0.0F) {
        throw std::invalid_argument("3D physics material values must be non-negative");
    }
    return component;
}

[[nodiscard]] std::string_view cameraProjectionName(const CameraProjection projection) {
    switch (projection) {
        case CameraProjection::Perspective: return "Perspective";
        case CameraProjection::Orthographic: return "Orthographic";
    }
    throw std::invalid_argument("unknown camera projection");
}

[[nodiscard]] CameraProjection parseCameraProjection(const Json& json) {
    const std::string value = json.get<std::string>();
    if (value == "Perspective") return CameraProjection::Perspective;
    if (value == "Orthographic") return CameraProjection::Orthographic;
    throw std::invalid_argument("unknown camera projection: " + value);
}

[[nodiscard]] Json serializeCamera(const CameraComponent& component) {
    return {
        {"Projection", cameraProjectionName(component.projection)},
        {"VerticalFieldOfView", serializedFloat(component.verticalFieldOfViewRadians)},
        {"OrthographicHeight", serializedFloat(component.orthographicHeight)},
        {"NearPlane", serializedFloat(component.nearPlane)},
        {"FarPlane", serializedFloat(component.farPlane)},
        {"ClearColor", vector4(component.clearColor)},
        {"Priority", component.priority},
        {"Active", component.active},
        {"ClearColorEnabled", component.clearColorEnabled},
        {"ClearDepthEnabled", component.clearDepthEnabled},
    };
}

[[nodiscard]] CameraComponent deserializeCamera(const Json& json) {
    const auto clearColor = floatArray<4>(json.at("ClearColor"));
    CameraComponent component{
        .projection = parseCameraProjection(json.at("Projection")),
        .verticalFieldOfViewRadians = finiteFloat(json.at("VerticalFieldOfView")),
        .orthographicHeight = finiteFloat(json.at("OrthographicHeight")),
        .nearPlane = finiteFloat(json.at("NearPlane")),
        .farPlane = finiteFloat(json.at("FarPlane")),
        .clearColor = {clearColor[0], clearColor[1], clearColor[2], clearColor[3]},
        .priority = json.at("Priority").get<std::int32_t>(),
        .active = json.at("Active").get<bool>(),
        .clearColorEnabled = json.at("ClearColorEnabled").get<bool>(),
        .clearDepthEnabled = json.at("ClearDepthEnabled").get<bool>(),
    };
    if (component.verticalFieldOfViewRadians <= 0.0F ||
        component.verticalFieldOfViewRadians >= 3.1415926536F ||
        component.orthographicHeight <= 0.0F || component.nearPlane <= 0.0F ||
        component.farPlane <= component.nearPlane) {
        throw std::invalid_argument("camera projection settings are invalid");
    }
    return component;
}

} // namespace

bool SceneSerializer::serialize(
    const std::filesystem::path& path,
    const SceneJsonFormat format
) const {
    m_lastError.clear();
    try {
        const SceneEnvironment& environment = m_scene.environment();
        renderer::Lighting environmentValidator;
        environmentValidator.setAmbientLight({
            .color = environment.ambientColor,
            .intensity = environment.ambientIntensity,
        });
        Json root{
            {"FormatVersion", sceneFormatVersion},
            {"Scene", m_scene.m_name},
            {"Environment", {
                {"AmbientColor", vector3(environment.ambientColor)},
                {"AmbientIntensity", serializedFloat(environment.ambientIntensity)},
            }},
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
                const std::filesystem::path texturePath = sprite.texture.valid()
                    ? sprite.texture.sourcePath()
                    : sprite.texturePath;
                entity["SpriteRenderer"] = {
                    {"Texture", texturePath.generic_string()},
                    {"Color", vector4(sprite.color)},
                    {"Tiling", vector2(sprite.tiling)},
                    {"SortingLayer", sprite.sortingLayer},
                };
                if (sprite.texture.valid()) {
                    entity["SpriteRenderer"]["TextureId"] =
                        std::to_string(sprite.texture.handle().id());
                }
            }
            if (registry.all_of<CameraComponent>(handle)) {
                entity["Camera"] = serializeCamera(
                    registry.get<CameraComponent>(handle)
                );
            }
            if (registry.all_of<ModelRendererComponent>(handle)) {
                const auto& model = registry.get<ModelRendererComponent>(handle);
                entity["ModelRenderer"] = {
                    {"Model", serializeAssetReference(model.model)},
                    {"Visible", model.visible},
                };
            }
            if (registry.all_of<AudioSourceComponent>(handle)) {
                const auto& source = registry.get<AudioSourceComponent>(handle);
                if (!std::isfinite(source.volume) || source.volume < 0.0F ||
                    !std::isfinite(source.pitch) || source.pitch <= 0.0F ||
                    !std::isfinite(source.minimumDistance) ||
                    !std::isfinite(source.maximumDistance) ||
                    source.minimumDistance < 0.0F ||
                    source.maximumDistance < source.minimumDistance) {
                    throw std::invalid_argument(
                        "Audio source volume and pitch must be valid"
                    );
                }
                entity["AudioSource"] = {
                    {"Clip", std::to_string(
                        source.clipAsset.valid()
                            ? source.clipAsset.handle().id()
                            : source.clip.id()
                    )},
                    {"Bus", audioBusName(source.bus)},
                    {"LoadMode", audioLoadModeName(source.loadMode)},
                    {"Volume", serializedFloat(source.volume)},
                    {"Pitch", serializedFloat(source.pitch)},
                    {"Looping", source.looping},
                    {"PlayOnStart", source.playOnStart},
                    {"Spatial", source.spatial},
                    {"Attenuation", attenuationName(source.attenuation)},
                    {"MinimumDistance", serializedFloat(source.minimumDistance)},
                    {"MaximumDistance", serializedFloat(source.maximumDistance)},
                };
                if (source.clipAsset.valid()) {
                    entity["AudioSource"]["ClipPath"] =
                        source.clipAsset.sourcePath().generic_string();
                }
            }
            if (registry.all_of<AudioListenerComponent>(handle)) {
                const auto& listener = registry.get<AudioListenerComponent>(handle);
                entity["AudioListener"] = {{"Active", listener.active}};
            }
            if (registry.all_of<LightComponent>(handle)) {
                entity["Light"] = serializeLight(
                    registry.get<LightComponent>(handle)
                );
            }
            if (registry.all_of<RigidBody2DComponent>(handle)) {
                entity["RigidBody2D"] = serializeRigidBody2D(
                    registry.get<RigidBody2DComponent>(handle)
                );
            }
            if (registry.all_of<Collider2DComponent>(handle)) {
                entity["Collider2D"] = serializeCollider2D(
                    registry.get<Collider2DComponent>(handle)
                );
            }
            if (registry.all_of<RigidBody3DComponent>(handle)) {
                entity["RigidBody3D"] = serializeRigidBody3D(
                    registry.get<RigidBody3DComponent>(handle)
                );
            }
            if (registry.all_of<Collider3DComponent>(handle)) {
                entity["Collider3D"] = serializeCollider3D(
                    registry.get<Collider3DComponent>(handle)
                );
            }
            if (registry.all_of<ScriptComponent>(handle)) {
                entity["Scripts"] = serializeScripts(
                    registry.get<ScriptComponent>(handle)
                );
            }
            if (registry.all_of<ParentComponent>(handle)) {
                entity["Parent"] = std::to_string(
                    registry.get<ParentComponent>(handle).parentUuid
                );
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
    if (m_writableScene == nullptr) {
        setError("Cannot deserialize into a read-only scene");
        return false;
    }

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
        SceneEnvironment loadedEnvironment;
        if (const auto environment = root.find("Environment");
            environment != root.end()) {
            const auto ambientColor = floatArray<3>(environment->at("AmbientColor"));
            loadedEnvironment.ambientColor = {
                ambientColor[0], ambientColor[1], ambientColor[2]
            };
            loadedEnvironment.ambientIntensity =
                environment->at("AmbientIntensity").get<float>();
            renderer::Lighting validator;
            validator.setAmbientLight({
                .color = loadedEnvironment.ambientColor,
                .intensity = loadedEnvironment.ambientIntensity,
            });
        }

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
                if (const auto textureId = sprite->find("TextureId");
                    textureId != sprite->end()) {
                    auto& component = loadedRegistry.get<SpriteRendererComponent>(handle);
                    component.texture = {
                        asset::AssetHandle<renderer::Texture2D>::fromId(
                            parseAssetId(*textureId)
                        ),
                        component.texturePath,
                    };
                }
            }
            if (const auto camera = serializedEntity.find("Camera");
                camera != serializedEntity.end()) {
                loadedRegistry.emplace<CameraComponent>(
                    handle,
                    deserializeCamera(*camera)
                );
            }
            if (const auto model = serializedEntity.find("ModelRenderer");
                model != serializedEntity.end()) {
                loadedRegistry.emplace<ModelRendererComponent>(
                    handle,
                    deserializeAssetReference<renderer::Model>(model->at("Model")),
                    model->at("Visible").get<bool>()
                );
            }
            if (const auto source = serializedEntity.find("AudioSource");
                source != serializedEntity.end()) {
                const float volume = source->at("Volume").get<float>();
                const float pitch = source->at("Pitch").get<float>();
                if (!std::isfinite(volume) || volume < 0.0F ||
                    !std::isfinite(pitch) || pitch <= 0.0F) {
                    throw std::invalid_argument(
                        "Audio source volume and pitch must be valid"
                    );
                }
                AudioSourceComponent component{
                    .clip = audio::AudioClipHandle::fromId(
                        parseAssetId(source->at("Clip"))
                    ),
                    .bus = parseAudioBus(source->at("Bus")),
                    .loadMode = parseAudioLoadMode(source->at("LoadMode")),
                    .volume = volume,
                    .pitch = pitch,
                    .looping = source->at("Looping").get<bool>(),
                    .playOnStart = source->at("PlayOnStart").get<bool>(),
                    .spatial = source->at("Spatial").get<bool>(),
                };
                if (const auto clipPath = source->find("ClipPath");
                    clipPath != source->end()) {
                    component.clipAsset = {
                        component.clip,
                        clipPath->get<std::string>(),
                    };
                }
                if (const auto attenuation = source->find("Attenuation");
                    attenuation != source->end()) {
                    component.attenuation = parseAttenuation(*attenuation);
                }
                if (const auto minimum = source->find("MinimumDistance");
                    minimum != source->end()) {
                    component.minimumDistance = minimum->get<float>();
                }
                if (const auto maximum = source->find("MaximumDistance");
                    maximum != source->end()) {
                    component.maximumDistance = maximum->get<float>();
                }
                if (!std::isfinite(component.minimumDistance) ||
                    !std::isfinite(component.maximumDistance) ||
                    component.minimumDistance < 0.0F ||
                    component.maximumDistance < component.minimumDistance) {
                    throw std::invalid_argument("Audio source distances must be valid");
                }
                loadedRegistry.emplace<AudioSourceComponent>(handle, std::move(component));
            }
            if (const auto listener = serializedEntity.find("AudioListener");
                listener != serializedEntity.end()) {
                loadedRegistry.emplace<AudioListenerComponent>(
                    handle,
                    listener->at("Active").get<bool>()
                );
            }
            if (const auto light = serializedEntity.find("Light");
                light != serializedEntity.end()) {
                loadedRegistry.emplace<LightComponent>(
                    handle,
                    deserializeLight(*light)
                );
            }
            if (const auto rigidBody = serializedEntity.find("RigidBody2D");
                rigidBody != serializedEntity.end()) {
                loadedRegistry.emplace<RigidBody2DComponent>(
                    handle,
                    deserializeRigidBody2D(*rigidBody)
                );
            }
            if (const auto collider = serializedEntity.find("Collider2D");
                collider != serializedEntity.end()) {
                loadedRegistry.emplace<Collider2DComponent>(
                    handle,
                    deserializeCollider2D(*collider)
                );
            }
            if (const auto rigidBody = serializedEntity.find("RigidBody3D");
                rigidBody != serializedEntity.end()) {
                loadedRegistry.emplace<RigidBody3DComponent>(
                    handle,
                    deserializeRigidBody3D(*rigidBody)
                );
            }
            if (const auto collider = serializedEntity.find("Collider3D");
                collider != serializedEntity.end()) {
                loadedRegistry.emplace<Collider3DComponent>(
                    handle,
                    deserializeCollider3D(*collider)
                );
            }
            if (const auto scripts = serializedEntity.find("Scripts");
                scripts != serializedEntity.end()) {
                loadedRegistry.emplace<ScriptComponent>(
                    handle,
                    deserializeScripts(*scripts)
                );
            }
            if (const auto parent = serializedEntity.find("Parent");
                parent != serializedEntity.end()) {
                const std::uint64_t parentUuid = parseAssetId(*parent);
                if (parentUuid == 0 || parentUuid == uuid) {
                    throw std::invalid_argument("Entity Parent must reference another entity");
                }
                loadedRegistry.emplace<ParentComponent>(handle, parentUuid);
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

        for (const auto [handle, uuid, relationship] : loadedRegistry.view<
             const UUIDComponent,
             const ParentComponent
        >().each()) {
            (void)handle;
            std::unordered_set<std::uint64_t> ancestry{uuid.uuid};
            std::uint64_t ancestor = relationship.parentUuid;
            while (ancestor != 0) {
                const auto found = loadedEntitiesByUuid.find(ancestor);
                if (found == loadedEntitiesByUuid.end()) {
                    throw std::invalid_argument("Entity Parent references a missing entity");
                }
                if (!ancestry.insert(ancestor).second) {
                    throw std::invalid_argument("Entity hierarchy contains a cycle");
                }
                const auto* next = loadedRegistry.try_get<ParentComponent>(found->second);
                ancestor = next ? next->parentUuid : 0;
            }
        }

        m_writableScene->m_registry = std::move(loadedRegistry);
        m_writableScene->m_entitiesByUuid = std::move(loadedEntitiesByUuid);
        m_writableScene->m_name = loadedSceneName;
        m_writableScene->m_environment = loadedEnvironment;
        ++m_writableScene->m_generation;
        if (m_writableScene->m_generation == 0) {
            ++m_writableScene->m_generation;
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

core::Result<void> SceneSerializer::serializeResult(
    const std::filesystem::path& path,
    const SceneJsonFormat format
) const {
    if (serialize(path, format)) return core::Result<void>::success();
    return core::Result<void>::failure({
        .severity = core::DiagnosticSeverity::Error,
        .code = "scene.serialize",
        .message = m_lastError,
        .path = path,
    });
}

core::Result<void> SceneSerializer::deserializeResult(
    const std::filesystem::path& path
) {
    if (deserialize(path)) return core::Result<void>::success();
    return core::Result<void>::failure({
        .severity = core::DiagnosticSeverity::Error,
        .code = "scene.deserialize",
        .message = m_lastError,
        .path = path,
    });
}

} // namespace vshade::scene
