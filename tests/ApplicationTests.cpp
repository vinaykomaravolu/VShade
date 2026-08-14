#include <catch2/catch_test_macros.hpp>

#include <core/Application.hpp>

#include <limits>
#include <stdexcept>

TEST_CASE("Application validates timing configuration", "[application]") {
    vshade::core::ApplicationConfig config;
    config.fixedDeltaTime = 0.0F;
    CHECK_THROWS_AS(vshade::core::Application(config), std::invalid_argument);

    config.fixedDeltaTime = 1.0F / 60.0F;
    config.maximumDeltaTime = std::numeric_limits<float>::quiet_NaN();
    CHECK_THROWS_AS(vshade::core::Application(config), std::invalid_argument);
}

TEST_CASE("Application rejects window access before startup", "[application]") {
    vshade::core::Application application;
    CHECK_THROWS_AS(application.getWindow(), std::logic_error);
}
