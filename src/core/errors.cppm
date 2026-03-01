module;
#include <format>
#include <source_location>
#include <stacktrace>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
export module core:errors;

import :log;
import :log_sinks;

export namespace core::inline errors
{
    auto assert(bool expression, const std::string &message = "", const std::source_location &loc = std::source_location::current()) -> void;

    class c_error : public std::runtime_error
    {
    public:
        explicit c_error(std::string_view message, std::stacktrace stacktrace = std::stacktrace::current(), std::source_location source_location = std::source_location::current());

        [[nodiscard]] auto stacktrace() const noexcept -> const std::stacktrace &;
        [[nodiscard]] auto source_location() const noexcept -> const std::source_location &;

    private:
        std::stacktrace m_stacktrace;
        std::source_location m_source_location;
    };

    using c_assertion_error = c_error;
} // namespace core::inline errors

// Implementation
export namespace std
{
    template <>
    struct formatter<core::errors::c_error, char>
    {
        constexpr auto parse(auto &ctx)
        {
            return ctx.begin();
        }

        auto format(const core::errors::c_error &error, auto &ctx) const
        {
            return std::format_to(ctx.out(),
                                  "Assertion Error: {}\n"
                                  "Source: {}:{} in {}\n"
                                  "\nPrinting Stacktrace:\n{}",
                                  error.what(),
                                  error.source_location().file_name(),
                                  error.source_location().line(),
                                  error.source_location().function_name(),
                                  error.stacktrace());
        }
    };
} // namespace std

namespace core::inline errors
{
    auto assert(bool expression, const std::string &message, const std::source_location &loc) -> void
    {
        static c_logger logger(log::e_log_level::error);
        logger.add_sink(core::create_console_sink());

        if (not expression)
        {
            std::string msg = "Assertion failed";
            if (not message.empty())
            {
                msg += ": " + message;
            }
            logger.log(e_log_level::fatal, "Assert", std::format("{} at {}:{} in {}", msg, loc.file_name(), loc.line(), loc.function_name()));
            logger.log(e_log_level::fatal, "Assert", std::format("Stacktrace:\n{}", std::stacktrace::current()));

            throw c_error(msg, std::stacktrace::current(), loc);
        }
    }

    c_error::c_error(std::string_view message, std::stacktrace stacktrace, std::source_location source_location)
        : std::runtime_error(std::string(message)),
          m_stacktrace(std::move(stacktrace)),
          m_source_location(source_location)
    {
    }

    auto c_error::stacktrace() const noexcept -> const std::stacktrace &
    {
        return m_stacktrace;
    }

    auto c_error::source_location() const noexcept -> const std::source_location &
    {
        return m_source_location;
    }
} // namespace core::inline errors
