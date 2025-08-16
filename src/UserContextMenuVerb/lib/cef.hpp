#pragma once

#define BITANY(value, bits) (((value) & (bits)) != 0)
#define BITALL(value, bits) (((value) & (bits)) == (bits))
#define BITMSK(value, mask, bits) (((value) & (mask)) == (bits))

template<typename T>
concept C_EnumBitmask =
    std::is_enum_v<T> &&
    requires { std::underlying_type_t<T>{}; };

template<C_EnumBitmask T>
constexpr T operator|(T lhs, T rhs) noexcept
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template<C_EnumBitmask T>
constexpr T operator&(T lhs, T rhs) noexcept
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template<C_EnumBitmask T>
constexpr T operator^(T lhs, T rhs) noexcept
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) ^ static_cast<U>(rhs));
}

template<C_EnumBitmask T>
constexpr T operator~(T e) noexcept
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(~static_cast<U>(e));
}

template<C_EnumBitmask T>
constexpr T& operator|=(T& lhs, T rhs) noexcept
{
    return lhs = lhs | rhs;
}

template<C_EnumBitmask T>
constexpr T& operator&=(T& lhs, T rhs) noexcept
{
    return lhs = lhs & rhs;
}

template<C_EnumBitmask T>
constexpr T& operator^=(T& lhs, T rhs) noexcept
{
    return lhs = lhs ^ rhs;
}
