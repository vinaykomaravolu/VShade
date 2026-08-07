#include <catch2/catch_test_macros.hpp>

#include <core/log.hpp>

namespace {

// Each test gets a fresh logging lifecycle, even if an assertion fails.
class LogSession final {
public:
    LogSession() {
        vshade::core::Log::Shutdown();
    }

    ~LogSession() {
        vshade::core::Log::Shutdown();
    }

    LogSession(const LogSession&) = delete;
    LogSession& operator=(const LogSession&) = delete;
};

} // namespace

TEST_CASE("Engine and game loggers are distinct", "[log]") {
    const LogSession session;

    vshade::core::Log::Initialize();
    const auto engine = vshade::core::Log::Engine();
    const auto game = vshade::core::Log::Game();

    REQUIRE(engine);
    REQUIRE(game);
    CHECK(engine != game);
    CHECK(engine->name() == "VShade");
    CHECK(game->name() == "Game");
}

TEST_CASE("Logging initialization is idempotent", "[log]") {
    const LogSession session;

    vshade::core::Log::Initialize();
    const auto first_engine = vshade::core::Log::Engine();
    const auto first_game = vshade::core::Log::Game();

    vshade::core::Log::Initialize();

    CHECK(vshade::core::Log::Engine() == first_engine);
    CHECK(vshade::core::Log::Game() == first_game);
}

TEST_CASE("Log level is applied to both loggers", "[log]") {
    const LogSession session;

    vshade::core::Log::SetLevel(spdlog::level::warn);

    CHECK(vshade::core::Log::Engine()->level() == spdlog::level::warn);
    CHECK(vshade::core::Log::Game()->level() == spdlog::level::warn);
}

TEST_CASE("Loggers can be recreated after shutdown", "[log]") {
    const LogSession session;

    vshade::core::Log::Initialize();
    REQUIRE(vshade::core::Log::Engine());
    REQUIRE(vshade::core::Log::Game());

    vshade::core::Log::Shutdown();

    CHECK_FALSE(spdlog::get("VShade"));
    CHECK_FALSE(spdlog::get("Game"));
    CHECK(vshade::core::Log::Engine());
    CHECK(vshade::core::Log::Game());
}
