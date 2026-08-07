#include <catch2/catch_test_macros.hpp>

#include <core/log.hpp>

namespace {

// Each test gets a fresh logging lifecycle, even if an assertion fails.
class LogSession final {
public:
    LogSession() {
        vshade::core::Log::shutdown();
    }

    ~LogSession() {
        vshade::core::Log::shutdown();
    }

    LogSession(const LogSession&) = delete;
    LogSession& operator=(const LogSession&) = delete;
};

} // namespace

TEST_CASE("Engine and game loggers are distinct", "[log]") {
    const LogSession session;

    vshade::core::Log::initialize();
    const auto engine = vshade::core::Log::engine();
    const auto game = vshade::core::Log::game();

    REQUIRE(engine);
    REQUIRE(game);
    CHECK(engine != game);
    CHECK(engine->name() == "VShade");
    CHECK(game->name() == "Game");
}

TEST_CASE("Logging initialization is idempotent", "[log]") {
    const LogSession session;

    vshade::core::Log::initialize();
    const auto first_engine = vshade::core::Log::engine();
    const auto first_game = vshade::core::Log::game();

    vshade::core::Log::initialize();

    CHECK(vshade::core::Log::engine() == first_engine);
    CHECK(vshade::core::Log::game() == first_game);
}

TEST_CASE("Log level is applied to both loggers", "[log]") {
    const LogSession session;

    vshade::core::Log::setLevel(spdlog::level::warn);

    CHECK(vshade::core::Log::engine()->level() == spdlog::level::warn);
    CHECK(vshade::core::Log::game()->level() == spdlog::level::warn);
}

TEST_CASE("Loggers can be recreated after shutdown", "[log]") {
    const LogSession session;

    vshade::core::Log::initialize();
    REQUIRE(vshade::core::Log::engine());
    REQUIRE(vshade::core::Log::game());

    vshade::core::Log::shutdown();

    CHECK_FALSE(spdlog::get("VShade"));
    CHECK_FALSE(spdlog::get("Game"));
    CHECK(vshade::core::Log::engine());
    CHECK(vshade::core::Log::game());
}
