module;
#include <any>
#include <cstdint>
#include <expected>
#include <string>
#include <utility>
#include <variant>
export module core:result;

export namespace core::inline results
{
    enum class e_domain : std::uint8_t
    {
        core,
        platform,
        engine,
        ui
    };

    enum class e_core_code : std::uint8_t
    {
        invalid_argument,
        out_of_range,
        null_pointer,
        log,
        not_initialized,
        already_initialized,
        unsupported_operation,
        timeout,
    };

    struct s_error
    {
        e_domain domain;
        std::any code;
        std::string message;
    };

    template <typename T>
    using result = std::expected<T, s_error>;

    using status = result<std::monostate>;

    inline auto failure(e_domain domain, std::any code, std::string message) -> std::unexpected<s_error>
    {
        return std::unexpected(s_error{ .domain = domain, .code = std::move(code), .message = std::move(message) });
    }
} // namespace core::inline results
