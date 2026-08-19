#include "UndoHistory.hpp"

#include <core/Log.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneSerializer.hpp>

#include <utility>

namespace editor {

std::optional<UndoHistory::Snapshot> UndoHistory::capture(
    const vshade::scene::Scene& scene,
    const vshade::scene::Entity selected
) {
    vshade::scene::SceneSerializer serializer(scene);
    std::string json = serializer.serializeToString(
        vshade::scene::SceneJsonFormat::Compact
    );
    if (json.empty()) {
        ENGINE_ERROR(
            "Failed to snapshot scene for undo: {}",
            serializer.lastError()
        );
        return std::nullopt;
    }

    Snapshot snapshot;
    snapshot.sceneJson = std::move(json);
    snapshot.selectedUuid = scene.valid(selected) ? selected.uuid() : 0;
    return snapshot;
}

bool UndoHistory::restore(
    vshade::scene::Scene& scene,
    const Snapshot& snapshot,
    vshade::scene::Entity& selected
) {
    vshade::scene::SceneSerializer serializer(scene);
    if (!serializer.deserializeFromString(snapshot.sceneJson)) {
        ENGINE_ERROR(
            "Failed to restore scene from undo snapshot: {}",
            serializer.lastError()
        );
        return false;
    }

    selected = snapshot.selectedUuid == 0
        ? vshade::scene::Entity{}
        : scene.findEntity(snapshot.selectedUuid);
    return true;
}

void UndoHistory::begin(
    const vshade::scene::Scene& scene,
    const vshade::scene::Entity selected
) {
    if (m_pending) {
        return;
    }
    m_pending = capture(scene, selected);
}

bool UndoHistory::commit(
    const vshade::scene::Scene& scene,
    const vshade::scene::Entity selected
) {
    if (!m_pending) {
        return false;
    }

    const auto after = capture(scene, selected);
    if (!after) {
        m_pending.reset();
        return false;
    }
    if (after->sceneJson == m_pending->sceneJson) {
        m_pending.reset();
        return false;
    }

    m_undo.push_back(Edit{
        .before = std::move(*m_pending),
        .after = *after,
    });
    m_pending.reset();
    m_redo.clear();
    while (m_undo.size() > maxDepth) {
        m_undo.erase(m_undo.begin());
    }
    return true;
}

void UndoHistory::cancel() noexcept {
    m_pending.reset();
}

bool UndoHistory::revert(
    vshade::scene::Scene& scene,
    vshade::scene::Entity& selected
) {
    if (!m_pending) {
        return false;
    }
    const Snapshot snapshot = std::move(*m_pending);
    m_pending.reset();
    return restore(scene, snapshot, selected);
}

bool UndoHistory::undo(
    vshade::scene::Scene& scene,
    vshade::scene::Entity& selected
) {
    if (m_pending) {
        return revert(scene, selected);
    }
    if (m_undo.empty()) {
        return false;
    }

    Edit& edit = m_undo.back();
    if (!restore(scene, edit.before, selected)) {
        return false;
    }
    m_redo.push_back(std::move(edit));
    m_undo.pop_back();
    return true;
}

bool UndoHistory::redo(
    vshade::scene::Scene& scene,
    vshade::scene::Entity& selected
) {
    if (m_pending || m_redo.empty()) {
        return false;
    }

    Edit& edit = m_redo.back();
    if (!restore(scene, edit.after, selected)) {
        return false;
    }
    m_undo.push_back(std::move(edit));
    m_redo.pop_back();
    return true;
}

void UndoHistory::clear() noexcept {
    m_pending.reset();
    m_undo.clear();
    m_redo.clear();
}

bool UndoHistory::canUndo() const noexcept {
    return m_pending.has_value() || !m_undo.empty();
}

bool UndoHistory::canRedo() const noexcept {
    return !m_pending.has_value() && !m_redo.empty();
}

bool UndoHistory::isRecording() const noexcept {
    return m_pending.has_value();
}

} // namespace editor
