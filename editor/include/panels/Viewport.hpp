#pragma once

#include "EditorCamera.hpp"

#include <memory>

namespace vshade::renderer {
class Framebuffer;
}

namespace editor {

class Viewport final {
public:
    Viewport();
    ~Viewport();

    Viewport(const Viewport&) = delete;
    Viewport& operator=(const Viewport&) = delete;

    void onUpdate(float deltaTime);
    void onImGuiRender();
    [[nodiscard]] bool wantsCursorCapture() const noexcept;

private:
    void renderScene();
    void resizeFramebuffer(float width, float height);

    std::unique_ptr<vshade::renderer::Framebuffer> m_framebuffer;
    EditorCamera m_editorCamera;
    bool m_hovered = false;
};

} // namespace editor
