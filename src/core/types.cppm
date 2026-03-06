module;
#include <format>
#include <functional>
#include <type_traits>
#include <utility>
export module core:types;

import :errors;

export namespace core::inline types
{
    template <typename T, typename Tag>
    class c_strong_type
    {
        T m_value;

    public:
        using value_type = T;
        using tag_type = Tag;

        constexpr c_strong_type() noexcept(std::is_nothrow_default_constructible_v<T>);
        constexpr explicit c_strong_type(T value) noexcept(std::is_nothrow_copy_constructible_v<T>);

        template <typename Self>
        [[nodiscard]] constexpr auto get(this Self &&self) noexcept -> auto &&;

        constexpr explicit operator T() const noexcept(std::is_nothrow_copy_constructible_v<T>);

        constexpr auto operator<=>(const c_strong_type &) const = default;
        constexpr auto operator==(const c_strong_type &) const -> bool = default;

        auto operator++() noexcept -> c_strong_type &
            requires std::is_integral_v<T>;

        auto operator++(int) noexcept -> c_strong_type
            requires std::is_integral_v<T>;

        auto operator--() noexcept -> c_strong_type &
            requires std::is_integral_v<T>;

        auto operator--(int) noexcept -> c_strong_type
            requires std::is_integral_v<T>;

        // Arithmetic operators (only for arithmetic types)
        auto operator+=(const c_strong_type &other) noexcept -> c_strong_type &
            requires std::is_arithmetic_v<T>;

        auto operator-=(const c_strong_type &other) noexcept -> c_strong_type &
            requires std::is_arithmetic_v<T>;

        auto operator*=(const c_strong_type &other) noexcept -> c_strong_type &
            requires std::is_arithmetic_v<T>;

        auto operator/=(const c_strong_type &other) noexcept -> c_strong_type &
            requires std::is_arithmetic_v<T>;
    };

    template <typename T>
    struct s_is_strong_type : std::false_type
    {
    };

    template <typename T, typename Tag>
    struct s_is_strong_type<core::types::c_strong_type<T, Tag>> : std::true_type
    {
    };

    template <typename T>
    concept is_strong_type = s_is_strong_type<std::remove_cv_t<T>>::value;
} // namespace core::inline types

export namespace std
{
    template <typename T>
    concept hashable = requires(const T val) { std::hash<T>{}(val); };

    template <typename T, typename Tag>
        requires hashable<T>
    struct hash<core::types::c_strong_type<T, Tag>>
    {
        auto operator()(const core::types::c_strong_type<T, Tag> &value) const noexcept(std::is_nothrow_invocable_v<std::hash<T>, const T &>) -> std::size_t
        {
            return std::hash<T>{}(value.get());
        }
    };

    template <typename T, typename Tag>
    struct formatter<core::types::c_strong_type<T, Tag>> : std::formatter<T>
    {
        using std::formatter<T>::parse;
        using std::formatter<T>::format;
    };
} // namespace std

// Implementation
namespace core::inline types
{

    template <typename T, typename Tag>
    constexpr c_strong_type<T, Tag>::c_strong_type() noexcept(std::is_nothrow_default_constructible_v<T>)
        : m_value()
    {
    }

    template <typename T, typename Tag>
    constexpr c_strong_type<T, Tag>::c_strong_type(T value) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : m_value(value)
    {
    }

    template <typename T, typename Tag>
    template <typename Self>
    constexpr auto c_strong_type<T, Tag>::get(this Self &&self) noexcept -> auto &&
    {
        return std::forward<Self>(self).m_value;
    }

    template <typename T, typename Tag>
    constexpr c_strong_type<T, Tag>::operator T() const noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        return m_value;
    }

    template <typename T, typename Tag>
    auto c_strong_type<T, Tag>::operator++() noexcept -> c_strong_type &
        requires std::is_integral_v<T>
    {
        ++m_value;
        return *this;
    }

    template <typename T, typename Tag>
    auto c_strong_type<T, Tag>::operator++(int) noexcept -> c_strong_type
        requires std::is_integral_v<T>
    {
        auto tmp = *this;
        ++m_value;
        return tmp;
    }

    template <typename T, typename Tag>
    auto c_strong_type<T, Tag>::operator--() noexcept -> c_strong_type &
        requires std::is_integral_v<T>
    {
        --m_value;
        return *this;
    }

    template <typename T, typename Tag>
    auto c_strong_type<T, Tag>::operator--(int) noexcept -> c_strong_type
        requires std::is_integral_v<T>
    {
        auto tmp = *this;
        --m_value;
        return tmp;
    }

    template <typename T, typename Tag>
    auto c_strong_type<T, Tag>::operator+=(const c_strong_type &other) noexcept -> c_strong_type &
        requires std::is_arithmetic_v<T>
    {
        m_value += other.m_value;
        return *this;
    }

    template <typename T, typename Tag>
    auto c_strong_type<T, Tag>::operator-=(const c_strong_type &other) noexcept -> c_strong_type &
        requires std::is_arithmetic_v<T>
    {
        m_value -= other.m_value;
        return *this;
    }

    template <typename T, typename Tag>
    auto c_strong_type<T, Tag>::operator*=(const c_strong_type &other) noexcept -> c_strong_type &
        requires std::is_arithmetic_v<T>
    {
        m_value *= other.m_value;
        return *this;
    }

    template <typename T, typename Tag>
    auto c_strong_type<T, Tag>::operator/=(const c_strong_type &other) noexcept -> c_strong_type &
        requires std::is_arithmetic_v<T>
    {
        m_value /= other.m_value;
        return *this;
    }
} // namespace core::inline types
