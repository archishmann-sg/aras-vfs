module;
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>
export module core:log;

import :result;

export namespace core::inline log
{
    enum class e_log_level : std::uint8_t
    {
        trace = 0,
        debug,
        info,
        warning,
        error,
        fatal,
    };

    struct s_log_entry
    {
        std::chrono::system_clock::time_point timestamp;
        e_log_level level = e_log_level::info;
        std::string module;
        std::string message;
    };

    class c_sink
    {
    public:
        virtual ~c_sink() = default;
        virtual auto write(const s_log_entry &entry) noexcept -> core::status = 0;
        virtual auto flush() noexcept -> core::status = 0;
    };

    class c_logger
    {
    public:
        explicit c_logger(e_log_level min_level = e_log_level::info) noexcept;
        ~c_logger();

        [[nodiscard]] auto min_level() const noexcept -> e_log_level;

        auto set_min_level(e_log_level level) noexcept -> void;
        auto set_sink_error_handler(std::function<void(std::shared_ptr<c_sink>)> handler) noexcept -> void;
        auto add_sink(std::shared_ptr<c_sink> sink) noexcept -> void;
        auto log(e_log_level level, std::string_view module, std::string_view message) noexcept -> void;
        auto flush() noexcept -> void;

    private:
        std::atomic<e_log_level> m_min_level;
        std::vector<std::shared_ptr<c_sink>> m_sinks;
        std::function<void(std::shared_ptr<c_sink>)> m_sink_error_handler;
        std::mutex m_mutex;
    };
} // namespace core::inline log

// Implementation
namespace core::inline log
{
    c_logger::c_logger(e_log_level min_level) noexcept
        : m_min_level(min_level)
    {
    }

    c_logger::~c_logger()
    {
        flush();
    }

    auto c_logger::set_min_level(e_log_level level) noexcept -> void
    {
        m_min_level.store(level, std::memory_order_relaxed);
    }

    auto c_logger::set_sink_error_handler(std::function<void(std::shared_ptr<c_sink>)> handler) noexcept -> void
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sink_error_handler = std::move(handler);
    }

    auto c_logger::min_level() const noexcept -> e_log_level
    {
        return m_min_level.load(std::memory_order_relaxed);
    }

    auto c_logger::add_sink(std::shared_ptr<c_sink> sink) noexcept -> void
    {
        if (not sink)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_sinks.push_back(std::move(sink));
    }

    auto c_logger::log(e_log_level level, std::string_view module, std::string_view message) noexcept -> void
    {
        if (level < m_min_level.load(std::memory_order_relaxed))
        {
            return;
        }

        s_log_entry entry;

        try
        {
            entry.timestamp = std::chrono::system_clock::now();
            entry.level = level;
            entry.module = module;
            entry.message = message;
        }
        catch (...)
        {
            return;
        };

        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto &sink : m_sinks)
        {
            if (not sink->write(entry))
            {
                if (m_sink_error_handler)
                {
                    m_sink_error_handler(sink);
                }
            }
        }
    }

    auto c_logger::flush() noexcept -> void
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto &sink : m_sinks)
        {
            if (not sink->flush())
            {
                if (m_sink_error_handler)
                {
                    m_sink_error_handler(sink);
                }
            }
        }
    }
} // namespace core::inline log
