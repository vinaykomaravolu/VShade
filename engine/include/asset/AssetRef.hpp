#pragma once

#include "asset/AssetReference.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

namespace vshade::asset {

/** @brief Owning runtime view of a resolved typed asset. */
template<typename Resource>
class AssetRef final {
public:
    AssetRef() = default;

    AssetRef(AssetReference<Resource> reference, std::shared_ptr<Resource> resource)
        : m_reference(std::move(reference)), m_resource(std::move(resource)) {
        if (m_reference.valid() != static_cast<bool>(m_resource)) {
            throw std::invalid_argument(
                "An AssetRef requires either both identity and resource, or neither"
            );
        }
    }

    [[nodiscard]] const AssetReference<Resource>& reference() const noexcept {
        return m_reference;
    }

    [[nodiscard]] AssetHandle<Resource> handle() const noexcept {
        return m_reference.handle();
    }

    [[nodiscard]] const std::shared_ptr<Resource>& shared() const noexcept {
        return m_resource;
    }

    [[nodiscard]] Resource* get() const noexcept { return m_resource.get(); }

    [[nodiscard]] Resource& operator*() const {
        if (!m_resource) {
            throw std::logic_error("Cannot dereference an empty AssetRef");
        }
        return *m_resource;
    }

    [[nodiscard]] Resource* operator->() const {
        if (!m_resource) {
            throw std::logic_error("Cannot dereference an empty AssetRef");
        }
        return m_resource.get();
    }

    explicit operator bool() const noexcept { return static_cast<bool>(m_resource); }

private:
    AssetReference<Resource> m_reference;
    std::shared_ptr<Resource> m_resource;
};

} // namespace vshade::asset
