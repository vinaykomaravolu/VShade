#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace vshade::core {

enum class DiagnosticSeverity { Info, Warning, Error };

/** @brief Structured content/runtime failure suitable for tools and logs. */
struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string code;
    std::string message;
    std::filesystem::path path;
};

template<typename Value>
class Result final {
public:
    static Result success(Value value) { return Result(std::move(value)); }
    static Result failure(Diagnostic diagnostic) {
        return Result(std::move(diagnostic));
    }

    [[nodiscard]] bool hasValue() const noexcept {
        return std::holds_alternative<Value>(m_storage);
    }
    explicit operator bool() const noexcept { return hasValue(); }
    [[nodiscard]] Value& value() { return std::get<Value>(m_storage); }
    [[nodiscard]] const Value& value() const { return std::get<Value>(m_storage); }
    [[nodiscard]] const Diagnostic& error() const {
        return std::get<Diagnostic>(m_storage);
    }

private:
    explicit Result(Value value) : m_storage(std::move(value)) {}
    explicit Result(Diagnostic diagnostic) : m_storage(std::move(diagnostic)) {}
    std::variant<Value, Diagnostic> m_storage;
};

template<>
class Result<void> final {
public:
    static Result success() { return Result(std::nullopt); }
    static Result failure(Diagnostic diagnostic) {
        return Result(std::move(diagnostic));
    }
    [[nodiscard]] bool hasValue() const noexcept { return !m_error.has_value(); }
    explicit operator bool() const noexcept { return hasValue(); }
    [[nodiscard]] const Diagnostic& error() const { return m_error.value(); }

private:
    explicit Result(std::optional<Diagnostic> error) : m_error(std::move(error)) {}
    explicit Result(Diagnostic diagnostic) : m_error(std::move(diagnostic)) {}
    std::optional<Diagnostic> m_error;
};

} // namespace vshade::core
