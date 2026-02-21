#include "cpu.hpp"

#include <cstdint>
#include <cassert>
#include <utility>
#include <format>

#include "bus.hpp"
#include "address.hpp"
#include "util.hpp"

namespace fxb
{
OpcodeNotImplementedException::OpcodeNotImplementedException(std::uint16_t extendedOpcode)
    : NotImplementedException(std::format("Opcode not implemented yet: 0x{:04X}", extendedOpcode)),
      extendedOpcode_(extendedOpcode)
{
}

Cpu::Cpu(Bus* bus) noexcept
    : bus_(bus)
{
    assert(bus);
    reset();
}

std::uint8_t Cpu::read(Address address)
{
    auto value = bus_->read(address);
    tick();
    return value;
}

void Cpu::write(Address address, std::uint8_t value)
{
    bus_->write(address, value);
    tick();
}

std::uint8_t Cpu::readNextByteAndAdvance() noexcept
{
    auto const oldPC = PC();
    ++PC_;
    return read(Address(oldPC));
}
void Cpu::execute(std::uint8_t opcode)
{
    switch (opcode)
    {
        case 0xCB:
        {
            std::uint8_t const prefixPart      = opcode;
            std::uint8_t const offsetPart      = readNextByteAndAdvance();
            std::uint16_t const extendedOpcode = ((prefixPart << 8) | offsetPart);
            executeExtended(extendedOpcode);
            break;
        }
        default: throw OpcodeNotImplementedException(opcode);
    }
}

void Cpu::executeExtended(std::uint16_t opcode)
{
    switch (opcode)
    {
        default: throw OpcodeNotImplementedException(opcode);
    }
}

void Cpu::tick()
{
    if (onTick)
    {
        onTick(this);
    }
}

void Cpu::setOnTickCallback(OnTickCallback callback)
{
    onTick = std::move(callback);
}

void Cpu::reset(CpuState const& state)
{
    A_  = state.A;
    B_  = state.B;
    C_  = state.C;
    D_  = state.D;
    E_  = state.E;
    F_  = state.F;
    H_  = state.H;
    L_  = state.L;
    SP_ = state.SP;
    PC_ = state.PC;
}

void Cpu::step()
{
    auto const opcode = readNextByteAndAdvance();
    execute(opcode);
}
} // namespace fxb