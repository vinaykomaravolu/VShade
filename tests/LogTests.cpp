#include <catch2/catch_test_macros.hpp>

#include <Core/Log.hpp>

namespace {

// Each test gets a fresh logging lifecycle, even if an assertion fails.
class LogSession final {
public:
    LogSession() {
        VShade::Log::Shutdown();
    }

    ~LogSession() {
        VShade::Log::Shutdown();
    }

    LogSession(const LogSession&) = delete;
    LogSession& operator=(const LogSession&) = delete;
};

} // namespace

TEST_CASE("Engine and game loggers are distinct", "[log]") {
    const LogSession session;

    VShade::Log::Initialize();
    const auto engine = VShade::Log::Engine();
    const auto game = VShade::Log::Game();

    REQUIRE(engine);
    REQUIRE(game);
    CHECK(engine != game);
    CHECK(engine->name() == "VShade");
    CHECK(game->name() == "Game");
}

TEST_CASE("Logging initialization is idempotent", "[log]") {
    const LogSession session;

    VShade::Log::Initialize();
    const auto first_engine = VShade::Log::Engine();
    const auto first_game = VShade::Log::Game();

    VShade::Log::Initialize();

    CHECK(VShade::Log::Engine() == first_engine);
    CHECK(VShade::Log::Game() == first_game);
}

TEST_CASE("Log level is applied to both loggers", "[log]") {
    const LogSession session;

    VShade::Log::SetLevel(spdlog::level::warn);

    CHECK(VShade::Log::Engine()->level() == spdlog::level::warn);
    CHECK(VShade::Log::Game()->level() == spdlog::level::warn);
}

TEST_CASE("Loggers can be recreated after shutdown", "[log]") {
    const LogSession session;

    VShade::Log::Initialize();
    REQUIRE(VShade::Log::Engine());
    REQUIRE(VShade::Log::Game());

    VShade::Log::Shutdown();

    CHECK_FALSE(spdlog::get("VShade"));
    CHECK_FALSE(spdlog::get("Game"));
    CHECK(VShade::Log::Engine());
    CHECK(VShade::Log::Game());
}
