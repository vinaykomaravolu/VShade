#include "ImGui/ImGuiTheme.hpp"

#include <imgui.h>

#include <cmath>

namespace editor {

void setImGuiTheme() {
    // Moonlight style by Madam-Herta from ImThemes.
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0F;
    style.DisabledAlpha = 0.45F;
    style.WindowPadding = ImVec2{12.0F, 12.0F};
    style.WindowRounding = 11.5F;
    style.WindowBorderSize = 0.0F;
    style.WindowMinSize = ImVec2{20.0F, 20.0F};
    style.WindowTitleAlign = ImVec2{0.5F, 0.5F};
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.ChildRounding = 0.0F;
    style.ChildBorderSize = 1.0F;
    style.PopupRounding = 0.0F;
    style.PopupBorderSize = 1.0F;
    style.FramePadding = ImVec2{20.0F, 3.4F};
    style.FrameRounding = 11.9F;
    style.FrameBorderSize = 0.0F;
    style.ItemSpacing = ImVec2{4.3F, 5.5F};
    style.ItemInnerSpacing = ImVec2{7.1F, 1.8F};
    style.CellPadding = ImVec2{12.1F, 9.2F};
    style.IndentSpacing = 0.0F;
    style.ColumnsMinSpacing = 4.9F;
    style.ScrollbarSize = 11.6F;
    style.ScrollbarRounding = 15.9F;
    style.GrabMinSize = 3.7F;
    style.GrabRounding = 20.0F;
    style.TabRounding = 0.0F;
    style.TabBorderSize = 0.0F;
    style.TabCloseButtonMinWidthSelected = 0.0F;
    style.TabCloseButtonMinWidthUnselected = 0.0F;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2{0.5F, 0.5F};
    style.SelectableTextAlign = ImVec2{0.0F, 0.0F};

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4{1.0F, 1.0F, 1.0F, 1.0F};
    colors[ImGuiCol_TextDisabled] = ImVec4{0.27450982F, 0.31764707F, 0.4509804F, 1.0F};
    colors[ImGuiCol_WindowBg] = ImVec4{0.078431375F, 0.08627451F, 0.101960786F, 1.0F};
    colors[ImGuiCol_ChildBg] = ImVec4{0.09411765F, 0.101960786F, 0.11764706F, 1.0F};
    colors[ImGuiCol_PopupBg] = ImVec4{0.078431375F, 0.08627451F, 0.101960786F, 1.0F};
    colors[ImGuiCol_Border] = ImVec4{0.15686275F, 0.16862746F, 0.19215687F, 1.0F};
    colors[ImGuiCol_BorderShadow] = ImVec4{0.078431375F, 0.08627451F, 0.101960786F, 1.0F};
    colors[ImGuiCol_FrameBg] = ImVec4{0.11372549F, 0.1254902F, 0.15294118F, 1.0F};
    colors[ImGuiCol_FrameBgHovered] = ImVec4{0.15686275F, 0.16862746F, 0.19215687F, 1.0F};
    colors[ImGuiCol_FrameBgActive] = ImVec4{0.15686275F, 0.16862746F, 0.19215687F, 1.0F};
    colors[ImGuiCol_TitleBg] = ImVec4{0.047058824F, 0.05490196F, 0.07058824F, 1.0F};
    colors[ImGuiCol_TitleBgActive] = ImVec4{0.047058824F, 0.05490196F, 0.07058824F, 1.0F};
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.078431375F, 0.08627451F, 0.101960786F, 1.0F};
    colors[ImGuiCol_MenuBarBg] = ImVec4{0.09803922F, 0.105882354F, 0.12156863F, 1.0F};
    colors[ImGuiCol_ScrollbarBg] = ImVec4{0.047058824F, 0.05490196F, 0.07058824F, 1.0F};
    colors[ImGuiCol_ScrollbarGrab] = ImVec4{0.11764706F, 0.13333334F, 0.14901961F, 1.0F};
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{0.15686275F, 0.16862746F, 0.19215687F, 1.0F};
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{0.11764706F, 0.13333334F, 0.14901961F, 1.0F};
    colors[ImGuiCol_CheckMark] = ImVec4{0.972549F, 1.0F, 0.49803922F, 1.0F};
    colors[ImGuiCol_SliderGrab] = ImVec4{0.972549F, 1.0F, 0.49803922F, 1.0F};
    colors[ImGuiCol_SliderGrabActive] = ImVec4{1.0F, 0.79607844F, 0.49803922F, 1.0F};
    colors[ImGuiCol_Button] = ImVec4{0.11764706F, 0.13333334F, 0.14901961F, 1.0F};
    colors[ImGuiCol_ButtonHovered] = ImVec4{0.18039216F, 0.1882353F, 0.19607843F, 1.0F};
    colors[ImGuiCol_ButtonActive] = ImVec4{0.15294118F, 0.15294118F, 0.15294118F, 1.0F};
    colors[ImGuiCol_Header] = ImVec4{0.14117648F, 0.16470589F, 0.20784314F, 1.0F};
    colors[ImGuiCol_HeaderHovered] = ImVec4{0.105882354F, 0.105882354F, 0.105882354F, 1.0F};
    colors[ImGuiCol_HeaderActive] = ImVec4{0.078431375F, 0.08627451F, 0.101960786F, 1.0F};
    colors[ImGuiCol_Separator] = ImVec4{0.12941177F, 0.14901961F, 0.19215687F, 1.0F};
    colors[ImGuiCol_SeparatorHovered] = ImVec4{0.15686275F, 0.18431373F, 0.2509804F, 1.0F};
    colors[ImGuiCol_SeparatorActive] = ImVec4{0.15686275F, 0.18431373F, 0.2509804F, 1.0F};
    colors[ImGuiCol_ResizeGrip] = ImVec4{0.14509805F, 0.14509805F, 0.14509805F, 1.0F};
    colors[ImGuiCol_ResizeGripHovered] = ImVec4{0.972549F, 1.0F, 0.49803922F, 1.0F};
    colors[ImGuiCol_ResizeGripActive] = ImVec4{1.0F, 1.0F, 1.0F, 1.0F};
    colors[ImGuiCol_Tab] = ImVec4{0.078431375F, 0.08627451F, 0.101960786F, 1.0F};
    colors[ImGuiCol_TabHovered] = ImVec4{0.11764706F, 0.13333334F, 0.14901961F, 1.0F};
    colors[ImGuiCol_TabActive] = ImVec4{0.11764706F, 0.13333334F, 0.14901961F, 1.0F};
    colors[ImGuiCol_TabUnfocused] = ImVec4{0.078431375F, 0.08627451F, 0.101960786F, 1.0F};
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.1254902F, 0.27450982F, 0.57254905F, 1.0F};
    colors[ImGuiCol_PlotLines] = ImVec4{0.52156866F, 0.6F, 0.7019608F, 1.0F};
    colors[ImGuiCol_PlotLinesHovered] = ImVec4{0.039215688F, 0.98039216F, 0.98039216F, 1.0F};
    colors[ImGuiCol_PlotHistogram] = ImVec4{0.88235295F, 0.79607844F, 0.56078434F, 1.0F};
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4{0.95686275F, 0.95686275F, 0.95686275F, 1.0F};
    colors[ImGuiCol_TableHeaderBg] = ImVec4{0.047058824F, 0.05490196F, 0.07058824F, 1.0F};
    colors[ImGuiCol_TableBorderStrong] = ImVec4{0.047058824F, 0.05490196F, 0.07058824F, 1.0F};
    colors[ImGuiCol_TableBorderLight] = ImVec4{0.0F, 0.0F, 0.0F, 1.0F};
    colors[ImGuiCol_TableRowBg] = ImVec4{0.11764706F, 0.13333334F, 0.14901961F, 1.0F};
    colors[ImGuiCol_TableRowBgAlt] = ImVec4{0.09803922F, 0.105882354F, 0.12156863F, 1.0F};
    colors[ImGuiCol_TextSelectedBg] = ImVec4{0.9372549F, 0.9372549F, 0.9372549F, 1.0F};
    colors[ImGuiCol_DragDropTarget] = ImVec4{0.49803922F, 0.5137255F, 1.0F, 1.0F};
    colors[ImGuiCol_NavHighlight] = ImVec4{0.26666668F, 0.2901961F, 1.0F, 1.0F};
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4{0.49803922F, 0.5137255F, 1.0F, 1.0F};
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4{0.19607843F, 0.1764706F, 0.54509807F, 0.5019608F};
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4{0.19607843F, 0.1764706F, 0.54509807F, 0.5019608F};
}

namespace ui {

float scaled(const float value) noexcept {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    return value * (viewport ? viewport->DpiScale : 1.0F);
}

ImVec2 scaled(const float x, const float y) noexcept {
    const float scale = scaled(1.0F);
    return {x * scale, y * scale};
}

ImVec4 color(const ColorRole role) noexcept {
    switch (role) {
        case ColorRole::Accent: return {0.9725F, 1.0F, 0.4980F, 1.0F};
        case ColorRole::Success: return {0.4392F, 0.8392F, 0.5804F, 1.0F};
        case ColorRole::Warning: return {1.0F, 0.7961F, 0.4980F, 1.0F};
        case ColorRole::Error: return {1.0F, 0.4314F, 0.4314F, 1.0F};
        case ColorRole::Destructive: return {0.7255F, 0.1804F, 0.2118F, 1.0F};
        case ColorRole::DestructiveHovered: return {0.9020F, 0.2510F, 0.2863F, 1.0F};
        case ColorRole::Muted: return {0.5216F, 0.6000F, 0.7020F, 1.0F};
        case ColorRole::Chrome: return {0.0471F, 0.0549F, 0.0706F, 1.0F};
        case ColorRole::SceneIconBackground:
            return {0.92F, 0.94F, 0.98F, 0.92F};
    }
    return {1.0F, 1.0F, 1.0F, 1.0F};
}

void pushDestructiveButtonStyle() {
    ImGui::PushStyleColor(ImGuiCol_Button, color(ColorRole::Destructive));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color(ColorRole::DestructiveHovered));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color(ColorRole::Destructive));
}

void popDestructiveButtonStyle() {
    ImGui::PopStyleColor(3);
}

void pushDestructiveTextStyle() {
    ImGui::PushStyleColor(ImGuiCol_Text, color(ColorRole::Error));
}

void popDestructiveTextStyle() {
    ImGui::PopStyleColor();
}

bool iconButton(
    const char* id,
    const Icon icon,
    const ImVec2 size,
    const char* tooltip,
    const bool active
) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, color(ColorRole::Accent));
        ImGui::PushStyleColor(ImGuiCol_Text, color(ColorRole::Chrome));
    }
    const bool clicked = ImGui::Button(id, size);
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    const ImVec2 center{
        (minimum.x + maximum.x) * 0.5F,
        (minimum.y + maximum.y) * 0.5F,
    };
    const float glyphScale = scaled(1.0F);
    const ImU32 glyphColor = ImGui::GetColorU32(ImGuiCol_Text);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    switch (icon) {
        case Icon::Play:
            drawList->AddTriangleFilled(
                {center.x - 5.0F * glyphScale, center.y - 7.0F * glyphScale},
                {center.x - 5.0F * glyphScale, center.y + 7.0F * glyphScale},
                {center.x + 7.0F * glyphScale, center.y},
                glyphColor
            );
            break;
        case Icon::Pause:
            drawList->AddRectFilled(
                {center.x - 6.0F * glyphScale, center.y - 7.0F * glyphScale},
                {center.x - 2.0F * glyphScale, center.y + 7.0F * glyphScale},
                glyphColor
            );
            drawList->AddRectFilled(
                {center.x + 2.0F * glyphScale, center.y - 7.0F * glyphScale},
                {center.x + 6.0F * glyphScale, center.y + 7.0F * glyphScale},
                glyphColor
            );
            break;
        case Icon::Step:
            drawList->AddTriangleFilled(
                {center.x - 7.0F * glyphScale, center.y - 7.0F * glyphScale},
                {center.x - 7.0F * glyphScale, center.y + 7.0F * glyphScale},
                {center.x + 4.0F * glyphScale, center.y},
                glyphColor
            );
            drawList->AddRectFilled(
                {center.x + 5.0F * glyphScale, center.y - 7.0F * glyphScale},
                {center.x + 8.0F * glyphScale, center.y + 7.0F * glyphScale},
                glyphColor
            );
            break;
        case Icon::Stop:
            drawList->AddRectFilled(
                {center.x - 6.0F * glyphScale, center.y - 6.0F * glyphScale},
                {center.x + 6.0F * glyphScale, center.y + 6.0F * glyphScale},
                glyphColor
            );
            break;
        case Icon::Translate: {
            const float radius = 7.0F * glyphScale;
            const float head = 3.0F * glyphScale;
            const float thickness = 1.5F * glyphScale;
            drawList->AddLine(
                {center.x - radius, center.y},
                {center.x + radius, center.y},
                glyphColor,
                thickness
            );
            drawList->AddLine(
                {center.x, center.y - radius},
                {center.x, center.y + radius},
                glyphColor,
                thickness
            );
            drawList->AddTriangleFilled(
                {center.x - radius, center.y},
                {center.x - radius + head, center.y - head},
                {center.x - radius + head, center.y + head},
                glyphColor
            );
            drawList->AddTriangleFilled(
                {center.x + radius, center.y},
                {center.x + radius - head, center.y - head},
                {center.x + radius - head, center.y + head},
                glyphColor
            );
            drawList->AddTriangleFilled(
                {center.x, center.y - radius},
                {center.x - head, center.y - radius + head},
                {center.x + head, center.y - radius + head},
                glyphColor
            );
            drawList->AddTriangleFilled(
                {center.x, center.y + radius},
                {center.x - head, center.y + radius - head},
                {center.x + head, center.y + radius - head},
                glyphColor
            );
            break;
        }
        case Icon::Rotate: {
            constexpr float arcStart = -2.55F;
            constexpr float arcEnd = 2.25F;
            const float radius = 7.0F * glyphScale;
            const float thickness = 1.7F * glyphScale;
            drawList->PathArcTo(center, radius, arcStart, arcEnd, 24);
            drawList->PathStroke(glyphColor, 0, thickness);
            const ImVec2 tip{
                center.x + std::cos(arcEnd) * radius,
                center.y + std::sin(arcEnd) * radius,
            };
            const float head = 3.2F * glyphScale;
            drawList->AddTriangleFilled(
                tip,
                {tip.x + head, tip.y - head * 0.25F},
                {tip.x + head * 0.15F, tip.y - head},
                glyphColor
            );
            break;
        }
        case Icon::Scale: {
            const float extent = 6.0F * glyphScale;
            const float handle = 2.4F * glyphScale;
            drawList->AddLine(
                {center.x - extent + handle, center.y + extent - handle},
                {center.x + extent - handle, center.y - extent + handle},
                glyphColor,
                1.7F * glyphScale
            );
            drawList->AddRectFilled(
                {center.x - extent, center.y + extent - handle * 2.0F},
                {center.x - extent + handle * 2.0F, center.y + extent},
                glyphColor,
                0.8F * glyphScale
            );
            drawList->AddRectFilled(
                {center.x + extent - handle * 2.0F, center.y - extent},
                {center.x + extent, center.y - extent + handle * 2.0F},
                glyphColor,
                0.8F * glyphScale
            );
            break;
        }
        case Icon::Reset: {
            constexpr float arcStart = -2.7F;
            constexpr float arcEnd = 2.05F;
            const float radius = 6.5F * glyphScale;
            drawList->PathArcTo(center, radius, arcStart, arcEnd, 22);
            drawList->PathStroke(glyphColor, 0, 1.7F * glyphScale);
            const ImVec2 tip{
                center.x + std::cos(arcStart) * radius,
                center.y + std::sin(arcStart) * radius,
            };
            const float head = 3.0F * glyphScale;
            drawList->AddTriangleFilled(
                tip,
                {tip.x + head, tip.y - head * 0.2F},
                {tip.x + head * 0.25F, tip.y + head},
                glyphColor
            );
            break;
        }
    }

    if (active) {
        ImGui::PopStyleColor(2);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return clicked;
}

} // namespace ui

} // namespace editor
