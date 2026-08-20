#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace editor {

/** Tracks the persisted identity and revision state of the edited scene. */
class EditorDocument final {
public:
    using Revision = std::uint64_t;

    void reset(std::filesystem::path path = {}, Revision revision = 0);
    void setPath(std::filesystem::path path);
    void setCurrentRevision(Revision revision) noexcept;
    void markSaved() noexcept;

    [[nodiscard]] const std::filesystem::path& path() const noexcept;
    [[nodiscard]] Revision savedRevision() const noexcept;
    [[nodiscard]] Revision currentRevision() const noexcept;
    [[nodiscard]] bool isDirty() const noexcept;
    [[nodiscard]] std::string displayName() const;

private:
    std::filesystem::path m_path;
    Revision m_savedRevision = 0;
    Revision m_currentRevision = 0;
};

} // namespace editor
