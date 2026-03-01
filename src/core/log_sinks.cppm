module;
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <print>
#include <string_view>
export module core:log_sinks;

import :log;
import :result;

// Implementation
namespace
{
    constexpr auto stringify_log_level(core::e_log_level level) noexcept -> std::string_view
    {
        switch (level)
        {
        case core::e_log_level::trace:
            return "TRACE     ";
        case core::e_log_level::debug:
            return "DEBUG     ";
        case core::e_log_level::info:
            return "INFO      ";
        case core::e_log_level::warning:
            return "WARNING   ";
        case core::e_log_level::error:
            return "ERROR     ";
        case core::e_log_level::fatal:
            return "FATAL     ";
        default:
            return "UNKNOWN   ";
        }
    }
} // anonymous namespace

namespace core::inline log_sinks
{
    class c_console_sink : public c_sink
    {
    public:
        auto write(const s_log_entry &entry) noexcept -> core::status override
        {
            const bool use_err = entry.level >= e_log_level::error;

            auto &stream = use_err ? std::cerr : std::cout;

            std::lock_guard lock(m_mutex);
            try
            {
                std::println(stream, "[{}]:[{}] - {}", stringify_log_level(entry.level), entry.module, entry.message);
                if (stream.fail())
                {
                    stream.clear();
                    return core::failure(e_domain::core, e_core_code::log, "Failed to write to console");
                }
                return {};
            }
            catch (const std::ios_base::failure &)
            {
                stream.clear();
                return core::failure(e_domain::core, e_core_code::log, "I/O error while writing to console");
            }
        }

        auto flush() noexcept -> core::status override
        {
            std::lock_guard lock(m_mutex);
            try
            {
                std::cout.flush();
                std::cerr.flush();
                if (std::cout.fail() or std::cerr.fail())
                {
                    std::cout.clear();
                    std::cerr.clear();
                    return core::failure(e_domain::core, e_core_code::log, "Failed to flush console");
                }
                return {};
            }
            catch (const std::ios_base::failure &)
            {
                std::cout.clear();
                std::cerr.clear();
                return core::failure(e_domain::core, e_core_code::log, "I/O error while flushing console");
            }
        }

    private:
        std::mutex m_mutex; // To ensure log entries don't interleave when writing to console
    };

    class c_file_sink : public c_sink
    {
    public:
        explicit c_file_sink(const std::filesystem::path &filename)
            : m_file(filename, std::ios::app | std::ios::out)
        {
            if (!m_file.is_open())
            {
                throw std::runtime_error("Failed to open log file: " + filename.string());
            }
            m_file.exceptions(std::ofstream::badbit);
        }

        // No copy or move, due to mutex and stream

        ~c_file_sink() override = default;
        c_file_sink(const c_file_sink &) = delete;
        auto operator=(const c_file_sink &) -> c_file_sink & = delete;
        c_file_sink(c_file_sink &&) = delete;
        auto operator=(c_file_sink &&) -> c_file_sink & = delete;

        auto write(const s_log_entry &entry) noexcept -> core::status override
        {
            std::lock_guard lock(m_mutex);

            if (not m_file.is_open() or not m_healthy)
            {
                return core::failure(e_domain::core, e_core_code::log, "File sink is not open or healthy");
            }

            try
            {
                std::println(m_file, "[{}]:[{}] - {}", stringify_log_level(entry.level), entry.module, entry.message);

                if (m_file.fail())
                {
                    m_file.clear();
                    return core::failure(e_domain::core, e_core_code::log, "Failed to write to log file");
                }
                return {};
            }
            catch (const std::ios_base::failure &)
            {
                m_healthy = false;
                m_file.clear();
                return core::failure(e_domain::core, e_core_code::log, "I/O error while writing to log file");
            }
        }
        auto flush() noexcept -> core::status override
        {
            std::lock_guard lock(m_mutex);

            if (not m_file.is_open() or not m_healthy)
            {
                return core::failure(e_domain::core, e_core_code::log, "File sink is not open or healthy");
            }

            try
            {
                m_file.flush();
                if (m_file.fail())
                {
                    m_file.clear();
                    return core::failure(e_domain::core, e_core_code::log, "Failed to flush log file");
                }
                return {};
            }
            catch (const std::ios_base::failure &)
            {
                m_healthy = false;
                m_file.clear();
                return core::failure(e_domain::core, e_core_code::log, "I/O error while flushing log file");
            }
        }

    private:
        bool m_healthy{ true };
        std::mutex m_mutex;
        std::ofstream m_file;
    };
} // namespace core::inline log_sinks

export namespace core::inline log_sinks
{
    [[nodiscard]] auto create_console_sink() -> std::unique_ptr<core::c_sink>
    {
        return std::make_unique<core::c_console_sink>();
    }

    [[nodiscard]] auto create_file_sink(std::string_view path) -> std::unique_ptr<core::c_sink>
    {
        return std::make_unique<core::c_file_sink>(path);
    }
} // namespace core::inline log_sinks
