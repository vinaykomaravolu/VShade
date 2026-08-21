#pragma once

#include <imgui.h>

namespace editor {

/** Applies the Moonlight Dear ImGui theme to the current ImGui context. */
void setImGuiTheme();

namespace ui {

enum class ColorRole {
    Accent,
    Success,
    Warning,
    Error,
    Destructive,
    DestructiveHovered,
    Muted,
    Chrome,
};

enum class Icon {
    Play,
    Pause,
    Step,
    Stop,
    Translate,
    Rotate,
    Scale,
};

struct Metrics final {
    static constexpr float toolbarHeight = 38.0F;
    static constexpr float statusBarHeight = 26.0F;
    static constexpr float controlHeight = 28.0F;
    static constexpr float toolbarButtonWidth = 42.0F;
    static constexpr float smallSpacing = 4.0F;
    static constexpr float spacing = 8.0F;
    static constexpr float chromePadding = 12.0F;
};

/** Returns a size scaled for the main viewport's current DPI. */
[[nodiscard]] float scaled(float value) noexcept;
[[nodiscard]] ImVec2 scaled(float x, float y) noexcept;
[[nodiscard]] ImVec4 color(ColorRole role) noexcept;

/** Semantic style scopes shared by confirmation dialogs and destructive menus. */
void pushDestructiveButtonStyle();
void popDestructiveButtonStyle();
void pushDestructiveTextStyle();
void popDestructiveTextStyle();

/** Draws a DPI-scaled editor glyph button with shared active/disabled styling. */
[[nodiscard]] bool iconButton(
    const char* id,
    Icon icon,
    ImVec2 size,
    const char* tooltip,
    bool active = false
);

} // namespace ui

} // namespace editor
