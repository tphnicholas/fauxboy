#ifndef FAUXBOY_ADDRESS_HPP
#define FAUXBOY_ADDRESS_HPP

#include <cstdint>
#include <compare>

namespace fxb
{
class Address
{
public:
    std::uint16_t value = 0;

public:
    constexpr Address() noexcept = default;
    constexpr explicit Address(std::uint16_t value) noexcept
        : value(value)
    {
    }

    [[nodiscard]] constexpr auto operator<=>(Address const& other) const noexcept { return (value <=> other.value); }
    [[nodiscard]] constexpr auto operator==(Address const& other) const noexcept { return (value == other.value); };

    [[nodiscard]] constexpr auto operator<=>(std::uint16_t other) const noexcept { return (*this <=> Address(other)); }
    [[nodiscard]] constexpr auto operator==(std::uint16_t other) const noexcept { return (*this == Address(other)); }

    constexpr Address& operator++() noexcept
    {
        ++value;
        return *this;
    }

    constexpr Address operator++(int) noexcept
    {
        auto const oldValue = value;
        ++value;
        return Address(oldValue);
    }

    constexpr Address& operator--() noexcept
    {
        --value;
        return *this;
    }

    constexpr Address operator--(int) noexcept
    {
        auto const oldValue = value;
        --value;
        return Address(oldValue);
    }
};
} // namespace fxb

#endif // FAUXBOY_ADDRESS_HPP