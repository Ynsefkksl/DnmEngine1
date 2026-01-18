#pragma once

#include <type_traits>

//I copied by vulkan hpp

template <typename FlagBitsType>
struct FlagTraits {
    static constexpr bool isBitmask = false;
};

template<typename T>
class Flags {
public:
    using Type = std::underlying_type_t<T>;

private:
    Type value;

public:
    constexpr Flags() noexcept : value(0) {}
    constexpr Flags(T bit) noexcept : value(static_cast<Type>(bit)) {}
    constexpr Flags(Type bits) noexcept : value(bits) {}

    constexpr Flags operator|(Flags o) const noexcept { return Flags(value | o.value); }
    constexpr Flags operator&(Flags o) const noexcept { return Flags(value & o.value); }
    constexpr Flags operator^(Flags o) const noexcept { return Flags(value ^ o.value); }
    constexpr Flags operator~()             const noexcept { return Flags(~value);    }

    constexpr Flags& operator|=(Flags o) noexcept { value |= o.value; return *this; }
    constexpr Flags& operator&=(Flags o) noexcept { value &= o.value; return *this; }
    constexpr Flags& operator^=(Flags o) noexcept { value ^= o.value; return *this; }

    [[nodiscard]] constexpr bool Any()  const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool None() const noexcept { return value == 0; }
    constexpr bool Has(T bit) const noexcept {
        return (value & static_cast<Type>(bit)) != 0;
    }

    constexpr explicit operator Type() const noexcept { return value; }
    constexpr explicit operator bool() const noexcept { return Any(); }
};

template <typename BitType, std::enable_if_t<FlagTraits<BitType>::isBitmask, bool> = true>
constexpr Flags<BitType> operator&( BitType lhs, BitType rhs ) noexcept {
    return Flags<BitType>( lhs ) & rhs;
}

template <typename BitType, std::enable_if_t<FlagTraits<BitType>::isBitmask, bool> = true>
constexpr Flags<BitType> operator|( BitType lhs, BitType rhs ) noexcept {
    return Flags<BitType>( lhs ) | rhs;
}

template <typename BitType, std::enable_if_t<FlagTraits<BitType>::isBitmask, bool> = true>
constexpr Flags<BitType> operator^( BitType lhs, BitType rhs ) noexcept {
    return Flags<BitType>( lhs ) ^ rhs;
}

template <typename BitType, std::enable_if_t<FlagTraits<BitType>::isBitmask, bool> = true>
constexpr Flags<BitType> operator~( BitType bit ) noexcept {
    return ~( Flags<BitType>( bit ) );
}