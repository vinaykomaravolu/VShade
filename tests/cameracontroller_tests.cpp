#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <math/vector.hpp>
#include <renderer/camera.hpp>
#include <renderer/cameracontroller.hpp>

#include <limits>
#include <stdexcept>

TEST_CASE("Fly camera controller preserves its initial camera pose", "[camera][controller]") {
    vshade::renderer::Camera camera;
    camera.setPerspective(0.785398163F, 1.0F, 0.1F, 100.0F);
    camera.lookAt(
        {2.0F, 1.0F, 4.0F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );

    vshade::renderer::FlyCameraController controller(camera);
    controller.update(0.0F);

    const vshade::math::Vec4 cameraPositionInView =
        camera.view() * vshade::math::Vec4{2.0F, 1.0F, 4.0F, 1.0F};
    CHECK(cameraPositionInView.x == Catch::Approx(0.0F).margin(0.0001F));
    CHECK(cameraPositionInView.y == Catch::Approx(0.0F).margin(0.0001F));
    CHECK(cameraPositionInView.z == Catch::Approx(0.0F).margin(0.0001F));
    CHECK(cameraPositionInView.w == Catch::Approx(1.0F).margin(0.0001F));
}

TEST_CASE("Fly camera controller validates movement settings", "[camera][controller]") {
    vshade::renderer::Camera camera;
    vshade::renderer::FlyCameraController controller(camera);

    controller.setMovementSpeed(8.0F);
    CHECK(controller.movementSpeed() == Catch::Approx(8.0F));
    CHECK_THROWS_AS(controller.setMovementSpeed(-1.0F), std::invalid_argument);
    CHECK_THROWS_AS(
        controller.update(std::numeric_limits<float>::quiet_NaN()),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        vshade::renderer::FlyCameraController(
            camera,
            {.mouseSensitivity = -0.1F}
        ),
        std::invalid_argument
    );
}
