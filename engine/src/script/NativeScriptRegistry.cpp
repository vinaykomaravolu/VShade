#include "script/NativeScriptRegistry.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace vshade::script {

struct NativeScriptRegistry::Impl {
    std::unordered_map<std::string, Factory> factories;
};

NativeScriptRegistry::NativeScriptRegistry()
    : m_impl(std::make_unique<Impl>()) {}

NativeScriptRegistry::~NativeScriptRegistry() = default;
NativeScriptRegistry::NativeScriptRegistry(NativeScriptRegistry&&) noexcept = default;
NativeScriptRegistry& NativeScriptRegistry::operator=(NativeScriptRegistry&&) noexcept = default;

void NativeScriptRegistry::registerFactory(std::string typeName, Factory factory) {
    if (typeName.empty()) {
        throw std::invalid_argument("Native script type name cannot be empty");
    }
    if (!factory) {
        throw std::invalid_argument("Native script factory cannot be empty");
    }
    if (!m_impl) {
        m_impl = std::make_unique<Impl>();
    }
    if (!m_impl->factories.emplace(std::move(typeName), std::move(factory)).second) {
        throw std::invalid_argument("Native script type name is already registered");
    }
}

bool NativeScriptRegistry::contains(const std::string_view typeName) const noexcept {
    if (!m_impl) {
        return false;
    }
    return std::ranges::any_of(
        m_impl->factories,
        [typeName](const auto& entry) { return entry.first == typeName; }
    );
}

std::unique_ptr<NativeScript> NativeScriptRegistry::create(
    const std::string_view typeName
) const {
    if (m_impl) {
        const auto factory = std::ranges::find_if(
            m_impl->factories,
            [typeName](const auto& entry) { return entry.first == typeName; }
        );
        if (factory != m_impl->factories.end()) {
            std::unique_ptr<NativeScript> instance = factory->second();
            if (!instance) {
                throw std::runtime_error("Native script factory returned null");
            }
            return instance;
        }
    }
    throw std::out_of_range("Native script type is not registered: " + std::string(typeName));
}

std::vector<std::string> NativeScriptRegistry::typeNames() const {
    std::vector<std::string> names;
    if (!m_impl) {
        return names;
    }
    names.reserve(m_impl->factories.size());
    for (const auto& [name, factory] : m_impl->factories) {
        (void)factory;
        names.push_back(name);
    }
    std::ranges::sort(names);
    return names;
}

void NativeScriptRegistry::clear() noexcept {
    if (m_impl) {
        m_impl->factories.clear();
    }
}

} // namespace vshade::script
