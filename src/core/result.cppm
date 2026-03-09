module;
#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <typeinfo>
#include <variant>
export module core:result;

namespace core::inline result
{
    struct s_type_id_base
    {
        virtual ~s_type_id_base() = default;
    };

    template <typename T>
    struct s_type_id : s_type_id_base
    {
        static const s_type_id s_instance;
    };

    template <typename T>
    const s_type_id<T> s_type_id<T>::s_instance{};

    using type_id_t = const s_type_id_base *;

    template <typename T>
    consteval auto get_type_id() -> type_id_t
    {
        return &s_type_id<T>::s_instance;
    }
} // namespace core::inline result

export namespace core::inline results
{
    enum class e_domain : std::uint8_t
    {
        core,
        fuse,
        winfsp,
        projfs,
        vfs,
        storage,
        aras,
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

    class c_code
    {
    public:
        template <typename Enum>
            requires std::is_enum_v<Enum>
        static constexpr auto from(Enum err_code_enum) -> c_code
        {
            c_code code{};
            code.m_type_id = get_type_id<Enum>();
            code.m_value = static_cast<std::uint32_t>(err_code_enum);
            return code;
        }

        template <typename Enum>
            requires std::is_enum_v<Enum>
        [[nodiscard]] auto is() const -> bool
        {
            return m_type_id == get_type_id<Enum>();
        }

        template <typename Enum>
            requires std::is_enum_v<Enum>
        auto as() const -> Enum
        {
            if (not is<Enum>())
            {
                throw std::runtime_error(std::format("Error code type mismatch: expected {}, got {}", typeid(Enum).name(), m_type_id ? typeid(*m_type_id).name() : "null"));
            }
            return static_cast<Enum>(m_value);
        }

    private:
        type_id_t m_type_id;
        std::uint32_t m_value;

        c_code() = default;
    };

    struct s_error
    {
        e_domain domain;
        c_code code;
        std::string message;
    };

    template <typename T>
    using result = std::expected<T, s_error>;

    using status = result<std::monostate>;

    template <typename Enum>
        requires std::is_enum_v<Enum>
    inline auto failure(e_domain domain, Enum code, const std::string &message) -> std::unexpected<s_error>
    {
        return std::unexpected(s_error{
            .domain = domain,
            .code = c_code::from(code),
            .message = message,
        });
    }
} // namespace core::inline results
