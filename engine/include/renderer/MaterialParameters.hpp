#pragma once

#include "math/Matrix.hpp"
#include "math/Vector.hpp"
#include "renderer/Texture.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace vshade::renderer {

/** @brief Value types that can be supplied to a user-defined shader uniform. */
using MaterialParameterValue = std::variant<
    int,
    float,
    math::Vec2,
    math::Vec3,
    math::Vec4,
    math::Mat4,
    std::shared_ptr<Texture2D>
>;

/**
 * @brief Named shader values stored by a material or copied into one draw command.
 *
 * Uniform locations remain owned and cached by Shader. Keeping the names here
 * makes this collection independent of one particular shader program.
 */
class MaterialParameters final {
public:
    using Values = std::unordered_map<std::string, MaterialParameterValue>;

    void set(std::string_view name, int value);
    void set(std::string_view name, float value);
    void set(std::string_view name, const math::Vec2& value);
    void set(std::string_view name, const math::Vec3& value);
    void set(std::string_view name, const math::Vec4& value);
    void set(std::string_view name, const math::Mat4& value);
    void set(std::string_view name, std::shared_ptr<Texture2D> value);

    /** @brief Removes a named value, returning true when it existed. */
    bool erase(std::string_view name);

    /** @brief Removes all values. */
    void clear() noexcept;

    [[nodiscard]] bool contains(std::string_view name) const;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    /** @brief Returns the stored values for renderer submission or inspection. */
    [[nodiscard]] const Values& values() const noexcept;

private:
    void setValue(std::string_view name, MaterialParameterValue value);

    Values m_values;
};

/** @brief Per-object shader values copied into a queued draw command. */
using DrawParameters = MaterialParameters;

} // namespace vshade::renderer
