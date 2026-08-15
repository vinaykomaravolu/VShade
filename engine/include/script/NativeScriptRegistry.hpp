#pragma once

#include "script/NativeScript.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vshade::script {

/** @brief Maps stable scene-file names to compiled C++ script factories. */
class NativeScriptRegistry final {
public:
    using Factory = std::function<std::unique_ptr<NativeScript>()>;

    NativeScriptRegistry();
    ~NativeScriptRegistry();

    NativeScriptRegistry(const NativeScriptRegistry&) = delete;
    NativeScriptRegistry& operator=(const NativeScriptRegistry&) = delete;
    NativeScriptRegistry(NativeScriptRegistry&&) noexcept;
    NativeScriptRegistry& operator=(NativeScriptRegistry&&) noexcept;

    /**
     * @brief Registers a default-constructible native script type.
     * @param typeName Stable name stored by ScriptBinding.
     * @throws std::invalid_argument If the name is empty or already registered.
     */
    template<typename Script>
        requires std::derived_from<Script, NativeScript> &&
                 std::default_initializable<Script>
    void registerType(std::string typeName) {
        registerFactory(
            std::move(typeName),
            []() -> std::unique_ptr<NativeScript> {
                return std::make_unique<Script>();
            }
        );
    }

    /**
     * @brief Registers a custom factory for a native script type.
     * @throws std::invalid_argument If the name or factory is empty, or the name exists.
     */
    void registerFactory(std::string typeName, Factory factory);

    /** @brief Reports whether a factory is registered under @p typeName. */
    [[nodiscard]] bool contains(std::string_view typeName) const noexcept;

    /**
     * @brief Creates a new script instance using a registered factory.
     * @throws std::out_of_range If no factory has the requested name.
     * @throws std::runtime_error If the factory returns null.
     */
    [[nodiscard]] std::unique_ptr<NativeScript> create(std::string_view typeName) const;

    /** @brief Returns all registered names in deterministic sorted order. */
    [[nodiscard]] std::vector<std::string> typeNames() const;

    /** @brief Removes every registered native script factory. */
    void clear() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::script
