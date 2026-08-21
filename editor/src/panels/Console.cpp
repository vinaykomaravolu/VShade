#include "panels/Console.hpp"
#include "ImGui/ImGuiTheme.hpp"

#include <core/Log.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <deque>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>
#include <spdlog/sinks/base_sink.h>

namespace editor {
namespace {

struct ConsoleEntry {
    std::string time;
    std::string logger;
    std::string message;
    spdlog::level::level_enum level = spdlog::level::info;
    std::size_t repeats = 1;
};

[[nodiscard]] std::string lowercase(std::string value) {
    std::ranges::transform(
        value,
        value.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        }
    );
    return value;
}

[[nodiscard]] ImVec4 levelColor(const spdlog::level::level_enum level) {
    switch (level) {
        case spdlog::level::trace: return ui::color(ui::ColorRole::Muted);
        case spdlog::level::debug: return ui::color(ui::ColorRole::Accent);
        case spdlog::level::warn: return ui::color(ui::ColorRole::Warning);
        case spdlog::level::err:
        case spdlog::level::critical: return ui::color(ui::ColorRole::Error);
        case spdlog::level::info:
        default: return ImGui::GetStyleColorVec4(ImGuiCol_Text);
    }
}

[[nodiscard]] std::size_t levelIndex(const spdlog::level::level_enum level) {
    return std::min<std::size_t>(static_cast<std::size_t>(level), 5);
}

} // namespace

struct Console::State {
    std::mutex mutex;
    std::deque<ConsoleEntry> entries;
    bool paused = false;
    bool scrollToBottom = false;
};

namespace {

class EditorConsoleSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit EditorConsoleSink(std::weak_ptr<Console::State> state)
        : m_state(std::move(state)) {}

protected:
    void sink_it_(const spdlog::details::log_msg& message) override {
        const std::shared_ptr<Console::State> state = m_state.lock();
        if (!state) {
            return;
        }

        ConsoleEntry entry;
        const auto timestamp = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()
        );
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &timestamp);
#else
        localtime_r(&timestamp, &local);
#endif
        std::ostringstream time;
        time << std::put_time(&local, "%H:%M:%S");
        entry.time = time.str();
        entry.logger.assign(message.logger_name.data(), message.logger_name.size());
        entry.message.assign(message.payload.data(), message.payload.size());
        entry.level = message.level;

        const std::scoped_lock lock(state->mutex);
        if (state->paused) {
            return;
        }
        if (!state->entries.empty()) {
            ConsoleEntry& previous = state->entries.back();
            if (previous.logger == entry.logger
                && previous.level == entry.level
                && previous.message == entry.message) {
                ++previous.repeats;
                previous.time = std::move(entry.time);
                state->scrollToBottom = true;
                return;
            }
        }
        constexpr std::size_t maximumEntries = 2000;
        if (state->entries.size() >= maximumEntries) {
            state->entries.pop_front();
        }
        state->entries.push_back(std::move(entry));
        state->scrollToBottom = true;
    }

    void flush_() override {}

private:
    std::weak_ptr<Console::State> m_state;
};

void removeSink(
    const std::shared_ptr<spdlog::logger>& logger,
    const std::shared_ptr<spdlog::sinks::sink>& sink
) {
    auto& sinks = logger->sinks();
    std::erase(sinks, sink);
}

} // namespace

Console::Console()
    : m_state(std::make_shared<State>()),
      m_sink(std::make_shared<EditorConsoleSink>(m_state)) {
    vshade::core::Log::engine()->sinks().push_back(m_sink);
    vshade::core::Log::game()->sinks().push_back(m_sink);
}

Console::~Console() {
    removeSink(vshade::core::Log::engine(), m_sink);
    removeSink(vshade::core::Log::game(), m_sink);
    m_sink.reset();
    m_state.reset();
}

void Console::onImGuiRender() {
    constexpr ImGuiWindowFlags panelFlags =
        ImGuiWindowFlags_NoCollapse;

    const bool visible = ImGui::Begin("Console", nullptr, panelFlags);
    if (!visible) {
        ImGui::End();
        return;
    }

    bool paused = false;
    {
        const std::scoped_lock lock(m_state->mutex);
        paused = m_state->paused;
    }
    if (ImGui::Button("Clear")) {
        const std::scoped_lock lock(m_state->mutex);
        m_state->entries.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button(paused ? "Resume" : "Pause")) {
        const std::scoped_lock lock(m_state->mutex);
        m_state->paused = !m_state->paused;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputTextWithHint(
        "##ConsoleSearch",
        "Search logs...",
        m_search.data(),
        m_search.size()
    );

    constexpr const char* levelNames[] = {
        "Trace", "Debug", "Info", "Warn", "Error", "Critical"
    };
    for (std::size_t index = 0; index < m_levels.size(); ++index) {
        if (index > 0) {
            ImGui::SameLine();
        }
        ImGui::Checkbox(levelNames[index], &m_levels[index]);
    }
    ImGui::Separator();

    std::vector<ConsoleEntry> entries;
    bool scrollToBottom = false;
    {
        const std::scoped_lock lock(m_state->mutex);
        entries.assign(m_state->entries.begin(), m_state->entries.end());
        scrollToBottom = std::exchange(m_state->scrollToBottom, false);
    }

    const std::string query = lowercase(m_search.data());
    const std::size_t warningCount = std::ranges::count_if(
        entries,
        [](const ConsoleEntry& entry) {
            return entry.level == spdlog::level::warn;
        }
    );
    const std::size_t errorCount = std::ranges::count_if(
        entries,
        [](const ConsoleEntry& entry) {
            return entry.level == spdlog::level::err
                || entry.level == spdlog::level::critical;
        }
    );
    ImGui::TextDisabled("%zu messages", entries.size());
    ImGui::SameLine();
    ImGui::TextColored(
        ui::color(ui::ColorRole::Warning),
        "%zu warnings",
        warningCount
    );
    ImGui::SameLine();
    ImGui::TextColored(
        ui::color(ui::ColorRole::Error),
        "%zu errors",
        errorCount
    );
    ImGui::BeginChild("##ConsoleEntries", {0.0F, 0.0F}, false);
    for (std::size_t entryIndex = 0; entryIndex < entries.size(); ++entryIndex) {
        const ConsoleEntry& entry = entries[entryIndex];
        if (!m_levels[levelIndex(entry.level)]) {
            continue;
        }
        const std::string searchable = lowercase(
            entry.logger + " " + entry.message
        );
        if (!query.empty() && searchable.find(query) == std::string::npos) {
            continue;
        }

        ImGui::PushID(static_cast<int>(entryIndex));
        ImGui::PushStyleColor(ImGuiCol_Text, levelColor(entry.level));
        ImGui::TextUnformatted(entry.time.c_str());
        ImGui::SameLine();
        ImGui::Text("[%s]", entry.logger.c_str());
        ImGui::SameLine();
        ImGui::TextWrapped("%s", entry.message.c_str());
        if (entry.repeats > 1) {
            ImGui::SameLine();
            ImGui::TextDisabled("x%zu", entry.repeats);
        }
        ImGui::PopStyleColor();
        // Text items do not have an ImGui ID, so the no-argument overload asserts
        // in debug builds. The pushed entry scope also keeps each popup unique.
        if (ImGui::BeginPopupContextItem("##ConsoleEntryContext")) {
            if (ImGui::MenuItem("Copy message")) {
                ImGui::SetClipboardText(entry.message.c_str());
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    if (m_autoScroll && scrollToBottom) {
        ImGui::SetScrollHereY(1.0F);
    }
    ImGui::EndChild();
    ImGui::End();
}

} // namespace editor
