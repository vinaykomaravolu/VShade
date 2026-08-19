#pragma once

#include "asset/AssetReference.hpp"

#include <cstdint>
#include <vector>

namespace vshade::scene {

class Prefab;

/** @brief Maps one prefab-template entity to its live instance counterpart. */
struct PrefabEntityLink {
    std::uint64_t prefabUuid = 0;
    std::uint64_t instanceUuid = 0;
};

/**
 * @brief Marks a scene subtree as a live instance of a .vsprefab asset.
 *
 * The root transform is an instance override. Other linked entities are
 * refreshed from the prefab when applyPrefab() runs.
 */
struct PrefabInstanceComponent {
    asset::AssetReference<Prefab> prefab;
    std::vector<PrefabEntityLink> entities;
};

} // namespace vshade::scene
