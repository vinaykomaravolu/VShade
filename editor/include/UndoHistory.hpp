#pragma once

#include <scene/Entity.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace vshade::scene {
class Scene;
}

namespace editor {

/**
 * @brief Undo/redo stack of compact scene JSON snapshots.
 *
 * Call begin() before a mutation and commit() after it. Unchanged scenes are
 * not recorded. Undo while an edit is open restores the begin() snapshot.
 */
class UndoHistory final {
public:
    static constexpr std::size_t maxDepth = 32;

    void begin(
        const vshade::scene::Scene& scene,
        vshade::scene::Entity selected
    );
    bool commit(
        const vshade::scene::Scene& scene,
        vshade::scene::Entity selected
    );
    void cancel() noexcept;
    bool revert(
        vshade::scene::Scene& scene,
        vshade::scene::Entity& selected
    );
    bool undo(
        vshade::scene::Scene& scene,
        vshade::scene::Entity& selected
    );
    bool redo(
        vshade::scene::Scene& scene,
        vshade::scene::Entity& selected
    );
    void clear() noexcept;

    [[nodiscard]] bool canUndo() const noexcept;
    [[nodiscard]] bool canRedo() const noexcept;
    [[nodiscard]] bool isRecording() const noexcept;
    [[nodiscard]] std::uint64_t currentRevision() const noexcept;

private:
    struct Snapshot {
        std::string sceneJson;
        std::uint64_t selectedUuid = 0;
        std::uint64_t revision = 0;
    };

    struct Edit {
        Snapshot before;
        Snapshot after;
    };

    [[nodiscard]] static std::optional<Snapshot> capture(
        const vshade::scene::Scene& scene,
        vshade::scene::Entity selected
    );
    static bool restore(
        vshade::scene::Scene& scene,
        const Snapshot& snapshot,
        vshade::scene::Entity& selected
    );

    std::optional<Snapshot> m_pending;
    std::vector<Edit> m_undo;
    std::vector<Edit> m_redo;
    std::uint64_t m_currentRevision = 0;
    std::uint64_t m_nextRevision = 1;
};

} // namespace editor
