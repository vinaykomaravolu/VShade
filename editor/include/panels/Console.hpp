#pragma once

#include <array>
#include <memory>

namespace spdlog::sinks {
class sink;
}

namespace editor {

class Console final {
public:
    struct State;

    Console();
    ~Console();

    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    void onImGuiRender();

private:
    std::shared_ptr<State> m_state;
    std::shared_ptr<spdlog::sinks::sink> m_sink;
    std::array<char, 192> m_search{};
    std::array<bool, 6> m_levels{true, true, true, true, true, true};
    bool m_autoScroll = true;
};

} // namespace editor
