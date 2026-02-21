#ifndef FAUXBOY_BUS_HPP
#define FAUXBOY_BUS_HPP

#include <cstdint>
#include <stdexcept>

#include "address.hpp"

namespace fxb
{
enum class MemoryAccessMode
{
    READ,
    WRITE
};

class BadMemoryAccessException : public std::runtime_error
{
private:
    Address address_;
    MemoryAccessMode accessMode_;

public:
    BadMemoryAccessException(Address address, MemoryAccessMode mode);

    [[nodiscard]] Address address() const noexcept { return address_; }
    [[nodiscard]] MemoryAccessMode accessMode() const noexcept { return accessMode_; }
};

class Bus
{
public:
    virtual ~Bus() = default;

    virtual std::uint8_t read(Address address)              = 0;
    virtual void write(Address address, std::uint8_t value) = 0;
};
} // namespace fxb

#endif // FAUXBOY_BUS_HPP