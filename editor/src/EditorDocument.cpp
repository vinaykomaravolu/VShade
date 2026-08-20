#include "EditorDocument.hpp"

#include <utility>

namespace editor {

void EditorDocument::reset(
    std::filesystem::path path,
    const Revision revision
) {
    m_path = std::move(path).lexically_normal();
    m_savedRevision = revision;
    m_currentRevision = revision;
}

void EditorDocument::setPath(std::filesystem::path path) {
    m_path = std::move(path).lexically_normal();
}

void EditorDocument::setCurrentRevision(const Revision revision) noexcept {
    m_currentRevision = revision;
}

void EditorDocument::markSaved() noexcept {
    m_savedRevision = m_currentRevision;
}

const std::filesystem::path& EditorDocument::path() const noexcept {
    return m_path;
}

EditorDocument::Revision EditorDocument::savedRevision() const noexcept {
    return m_savedRevision;
}

EditorDocument::Revision EditorDocument::currentRevision() const noexcept {
    return m_currentRevision;
}

bool EditorDocument::isDirty() const noexcept {
    return m_currentRevision != m_savedRevision;
}

std::string EditorDocument::displayName() const {
    if (m_path.empty()) {
        return "Untitled";
    }
    const std::string name = m_path.filename().generic_string();
    return name.empty() ? "Untitled" : name;
}

} // namespace editor
