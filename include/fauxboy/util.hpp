#ifndef FAUXBOY_UTIL_HPP
#define FAUXBOY_UTIL_HPP

#include <cstdint>
#include <concepts>
#include <stdexcept>
#include <string>

namespace fxb
{
class NotImplementedException : public std::runtime_error
{
public:
    explicit NotImplementedException(std::string const& reason)
        : std::runtime_error(reason)
    {
    }
};

template <std::integral T>
inline constexpr std::uint8_t getLower(T value) noexcept
{
    return (value & 0xFF);
}

template <std::integral T>
inline constexpr std::uint8_t getUpper(T value) noexcept
{
    constexpr auto shift = ((sizeof(T) * 8) - 8);
    return ((value >> shift) & 0xFF);
}

template <std::integral T>
inline constexpr void setLower(T& value, std::uint8_t byte) noexcept
{
    constexpr T mask = 0xFF;
    value            = ((value & ~mask) | byte);
}

template <std::integral T>
inline constexpr void setUpper(T& value, std::uint8_t byte) noexcept
{
    constexpr int shift = ((sizeof(T) * 8) - 8);
    constexpr T mask    = (0xFF << shift);
    value               = ((value & ~mask) | (byte << shift));
}
} // namespace fxb

#endif // FAUXBOY_UTIL_HPP