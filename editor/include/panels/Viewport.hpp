#pragma once

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

    void onImGuiRender();

private:
    void renderScene();
    void resizeFramebuffer(float width, float height);

    std::unique_ptr<vshade::renderer::Framebuffer> m_framebuffer;
};

} // namespace editor
