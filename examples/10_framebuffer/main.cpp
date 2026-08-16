#include <common/ExampleSupport.hpp>
#include <core/Log.hpp>

#if defined(_MSC_VER)
    #define _CRT_SECURE_NO_WARNINGS
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

class FramebufferExample final : public vshade::Application {
public:
    FramebufferExample() : Application(vshade::examples::config("10 - Framebuffer")) {}
protected:
    void onStart() override {
        m_framebuffer = std::make_unique<vshade::renderer::Framebuffer>(320, 180);
        m_camera.setOrthographic(-2.0F, 2.0F, -1.125F, 1.125F, -1.0F, 1.0F);
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override {
        m_framebuffer->bind();
        vshade::examples::clear({0.15F, 0.03F, 0.2F, 1.0F});
        vshade::renderer::Renderer2D::beginScene(m_camera);
        vshade::renderer::Renderer2D::drawQuad(
            vshade::math::Transform{}, {0.2F, 0.9F, 0.5F, 1.0F}
        );
        vshade::renderer::Renderer2D::endScene();
        if (!m_saved) {
            savePng();
            m_saved = true;
        }
        vshade::renderer::Framebuffer::unbind();
        vshade::examples::clear({0.03F, 0.08F, 0.12F, 1.0F});
    }
private:
    void savePng() const {
        const std::vector<std::uint8_t> pixels = m_framebuffer->readPixels();
        const std::filesystem::path output =
            std::filesystem::path(VSHADE_EXAMPLE_OUTPUT_DIR) / "framebuffer.png";
        std::filesystem::create_directories(output.parent_path());

        constexpr int channelCount = 4;
        const int width = static_cast<int>(m_framebuffer->width());
        const int height = static_cast<int>(m_framebuffer->height());
        const int written = stbi_write_png(
            output.string().c_str(),
            width,
            height,
            channelCount,
            pixels.data(),
            width * channelCount
        );
        if (written == 0) {
            throw std::runtime_error("Failed to write framebuffer PNG: " + output.string());
        }
        GAME_INFO("Saved offscreen framebuffer to {}", output.string());
    }

    std::unique_ptr<vshade::renderer::Framebuffer> m_framebuffer;
    vshade::renderer::Camera m_camera;
    bool m_saved = false;
};
VSHADE_GAME(FramebufferExample)
