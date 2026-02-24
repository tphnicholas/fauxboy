#ifndef FAUXBOY_REGISTER_HPP
#define FAUXBOY_REGISTER_HPP

#include <cstdint>
#include <type_traits>
#include <concepts>
#include <cassert>

#include "util.hpp"

namespace fxb
{
template <std::integral T>
class Register
{
protected:
    T value;

public:
    explicit Register(T value = 0) noexcept
        : value(value)
    {
    }

    Register(Register const& other)     = delete;
    Register operator=(Register const&) = delete;

    Register(Register&& other)      = delete;
    Register& operator=(Register&&) = delete;

    virtual ~Register() = default;

    [[nodiscard]] std::uint8_t lower() const noexcept { return getLower(value); }
    [[nodiscard]] std::uint8_t upper() const noexcept { return getUpper(value); }

    void setLower(std::uint8_t byte) noexcept { fxb::setLower(value, byte); }
    void setUpper(std::uint8_t byte) noexcept { fxb::setUpper(value, byte); }

    [[nodiscard]] T operator()() const noexcept { return value; }

    Register& operator=(T newValue) noexcept
    {
        value = newValue;
        return *this;
    }

    Register& operator++() noexcept
    {
        ++value;
        return *this;
    }

    Register& operator--() noexcept
    {
        --value;
        return *this;
    }
};

using ByteRegister  = Register<std::uint8_t>;
using ShortRegister = Register<std::uint16_t>;

class RegisterPairView
{
private:
    ByteRegister* upper_;
    ByteRegister* lower_;

public:
    RegisterPairView(ByteRegister* upper, ByteRegister* lower) noexcept
        : upper_(upper),
          lower_(lower)

    {
        assert(upper);
        assert(lower);
    }

    [[nodiscard]] decltype(auto) upper(this auto&& self) noexcept { return *(self.upper_); }
    [[nodiscard]] decltype(auto) lower(this auto&& self) noexcept { return *(self.lower_); }

    [[nodiscard]] virtual std::uint16_t operator()() const noexcept
    {
        auto const hi = upper()();
        auto const lo = lower()();
        return ((hi << 8) | lo);
    }
};

template <typename T,
          typename U = std::conditional_t<std::is_enum_v<T>, std::underlying_type<T>, std::type_identity<T>>::type>
    requires(std::is_enum_v<T> || std::is_integral_v<T>)
class FlagRegister final : public Register<U>
{
public:
    using Register<U>::operator=;

    FlagRegister() noexcept
        : Register<U>(0)
    {
    }

    explicit FlagRegister(T flag) noexcept
        : Register<U>(static_cast<U>(flag))
    {
    }

    void setFlag(T rawFlag, bool shouldSet = true) noexcept
    {
        auto const flag = static_cast<U>(rawFlag);
        auto& value     = Register<U>::value;
        value           = ((value & ~flag) | (flag * shouldSet));
    }

    void toggleFlag(T rawFlag) noexcept { setFlag(rawFlag, !isSet(rawFlag)); }

    [[nodiscard]] bool isSet(T rawFlag) const noexcept
    {
        auto const flag  = static_cast<U>(rawFlag);
        auto const value = Register<U>::value;
        return ((value & flag) != 0);
    }
};
} // namespace fxb

#endif // FAUXBOY_REGISTER_HPP