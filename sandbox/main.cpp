#include <exception>

#include <VShade/VShade.hpp>

int main() {
    try {
        VShade::Log::initialize();
        VSHADE_INFO("Starting the VShade sandbox");

        VShade::Window window({
            .title = "VShade Sandbox",
            .width = 1280,
            .height = 720,
            .vertical_sync = true,
        });

        while (!window.should_close()) {
            window.poll_events();

            if (window.is_key_pressed(VShade::Key::Escape)) {
                window.request_close();
            }

            window.clear(0.06F, 0.07F, 0.10F);
            window.swap_buffers();
        }

        VSHADE_INFO("Stopping the VShade sandbox");
        VShade::Log::shutdown();
        return 0;
    } catch (const std::exception& error) {
        VSHADE_CRITICAL("Fatal error: {}", error.what());
        VShade::Log::shutdown();
        return 1;
    }
}
