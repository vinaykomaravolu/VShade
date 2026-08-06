#include <catch2/catch_test_macros.hpp>

#include <VShade/Log.hpp>

namespace {

// Each test gets a fresh logging lifecycle, even if an assertion fails.
class LogSession final {
public:
    LogSession() {
        VShade::Log::shutdown();
    }

    ~LogSession() {
        VShade::Log::shutdown();
    }

    LogSession(const LogSession&) = delete;
    LogSession& operator=(const LogSession&) = delete;
};

} // namespace

TEST_CASE("Engine and application loggers are distinct", "[log]") {
    const LogSession session;

    VShade::Log::initialize();
    const auto engine = VShade::Log::engine();
    const auto client = VShade::Log::client();

    REQUIRE(engine);
    REQUIRE(client);
    CHECK(engine != client);
    CHECK(engine->name() == "VShade");
    CHECK(client->name() == "Application");
}

TEST_CASE("Logging initialization is idempotent", "[log]") {
    const LogSession session;

    VShade::Log::initialize();
    const auto first_engine = VShade::Log::engine();
    const auto first_client = VShade::Log::client();

    VShade::Log::initialize();

    CHECK(VShade::Log::engine() == first_engine);
    CHECK(VShade::Log::client() == first_client);
}

TEST_CASE("Log level is applied to both loggers", "[log]") {
    const LogSession session;

    VShade::Log::set_level(spdlog::level::warn);

    CHECK(VShade::Log::engine()->level() == spdlog::level::warn);
    CHECK(VShade::Log::client()->level() == spdlog::level::warn);
}

TEST_CASE("Loggers can be recreated after shutdown", "[log]") {
    const LogSession session;

    VShade::Log::initialize();
    REQUIRE(VShade::Log::engine());
    REQUIRE(VShade::Log::client());

    VShade::Log::shutdown();

    CHECK_FALSE(spdlog::get("VShade"));
    CHECK_FALSE(spdlog::get("Application"));
    CHECK(VShade::Log::engine());
    CHECK(VShade::Log::client());
}
