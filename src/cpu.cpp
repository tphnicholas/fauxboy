#include "cpu.hpp"

#include <cstdint>
#include <cassert>
#include <utility>
#include <format>
#include <stdexcept>

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

IllegalOpcodeException::IllegalOpcodeException(std::uint16_t extendedOpcode)
    : std::runtime_error(std::format("Illegal opcode requested: 0x{:04X}", extendedOpcode)),
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
        case 0x00:
        {
            break;
        }
        case 0x01:
        {
            BC_.lower() = readNextByteAndAdvance();
            BC_.upper() = readNextByteAndAdvance();
            break;
        }
        case 0x02:
        {
            write(Address(BC()), A());
            break;
        }
        case 0x03:
        {
            INC(BC_);
            break;
        }
        case 0x04:
        {
            INC(B_);
            break;
        }
        case 0x05:
        {
            DEC(B_);
            break;
        }
        case 0x06:
        {
            B_ = readNextByteAndAdvance();
            break;
        }
        case 0x07:
        {
            std::uint8_t const value  = A();
            std::uint8_t const carry  = (value & 0x80);
            std::uint8_t const result = ((value << 1) | (carry >> 7));

            A_ = result;
            F_.setFlag(Flag::ZERO, false);
            F_.setFlag(Flag::NEGATIVE, false);
            F_.setFlag(Flag::HALF_CARRY, false);
            F_.setFlag(Flag::CARRY, carry != 0);
            break;
        }
        case 0x08:
        {
            std::uint8_t const lo = readNextByteAndAdvance();
            std::uint8_t const hi = readNextByteAndAdvance();
            auto const address    = Address((hi << 8) | lo);
            write(address, SP_.lower());
            write(Address(address.value + 1), SP_.upper());
            break;
        }
        case 0x09:
        {
            ADD(HL_, BC());
            break;
        }
        case 0x0A:
        {
            A_ = read(Address(BC()));
            break;
        }
        case 0x0B:
        {
            DEC(BC_);
            break;
        }
        case 0x0C:
        {
            INC(C_);
            break;
        }
        case 0x0D:
        {
            DEC(C_);
            break;
        }
        case 0x0E:
        {
            C_ = readNextByteAndAdvance();
            break;
        }
        case 0x0F:
        {
            std::uint8_t const value  = A();
            std::uint8_t const carry  = (value & 0x01);
            std::uint8_t const result = ((value >> 1) | (carry << 7));

            A_ = result;
            F_.setFlag(Flag::ZERO, false);
            F_.setFlag(Flag::NEGATIVE, false);
            F_.setFlag(Flag::HALF_CARRY, false);
            F_.setFlag(Flag::CARRY, carry != 0);
            break;
        }
        case 0x10:
        {
            // TODO: Implement STOP
            // SingleStepTests expects 3 m-cycles here while the gbops table lists it as a 1 m-cycle instruction
            // follow the test suite for now
            tick();
            tick();
            break;
        }
        case 0x11:
        {
            DE_.lower() = readNextByteAndAdvance();
            DE_.upper() = readNextByteAndAdvance();
            break;
        }
        case 0x12:
        {
            write(Address(DE()), A());
            break;
        }
        case 0x13:
        {
            INC(DE_);
            break;
        }
        case 0x14:
        {
            INC(D_);
            break;
        }
        case 0x15:
        {
            DEC(D_);
            break;
        }
        case 0x16:
        {
            D_ = readNextByteAndAdvance();
            break;
        }
        case 0x17:
        {
            std::uint8_t const value  = A();
            std::uint8_t const carry  = (value & 0x80);
            std::uint8_t const result = ((value << 1) | F_.isSet(Flag::CARRY));

            A_ = result;
            F_.setFlag(Flag::ZERO, false);
            F_.setFlag(Flag::NEGATIVE, false);
            F_.setFlag(Flag::HALF_CARRY, false);
            F_.setFlag(Flag::CARRY, carry != 0);
            break;
        }
        case 0x18:
        {
            JR();
            break;
        }
        case 0x19:
        {
            ADD(HL_, DE());
            break;
        }
        case 0x1A:
        {
            A_ = read(Address(DE()));
            break;
        }
        case 0x1B:
        {
            DEC(DE_);
            break;
        }
        case 0x1C:
        {
            INC(E_);
            break;
        }
        case 0x1D:
        {
            DEC(E_);
            break;
        }
        case 0x1E:
        {
            E_ = readNextByteAndAdvance();
            break;
        }
        case 0x1F:
        {
            std::uint8_t const value  = A();
            std::uint8_t const carry  = (value & 0x01);
            std::uint8_t const result = ((value >> 1) | (F_.isSet(Flag::CARRY) << 7));

            A_ = result;
            F_.setFlag(Flag::ZERO, false);
            F_.setFlag(Flag::NEGATIVE, false);
            F_.setFlag(Flag::HALF_CARRY, false);
            F_.setFlag(Flag::CARRY, carry != 0);
            break;
        }
        case 0x20:
        {
            JR(!F_.isSet(Flag::ZERO));
            break;
        }
        case 0x21:
        {
            HL_.lower() = readNextByteAndAdvance();
            HL_.upper() = readNextByteAndAdvance();
            break;
        }
        case 0x22:
        {
            auto const address        = Address(HL());
            std::uint16_t const value = (HL() + 1);

            HL_.lower() = getLower(value);
            HL_.upper() = getUpper(value);
            write(address, A());
            break;
        }
        case 0x23:
        {
            INC(HL_);
            break;
        }
        case 0x24:
        {
            INC(H_);
            break;
        }
        case 0x25:
        {
            DEC(H_);
            break;
        }
        case 0x26:
        {
            H_ = readNextByteAndAdvance();
            break;
        }
        case 0x27:
        {
            std::uint8_t shift           = 0;
            std::uint16_t extendedResult = 0;

            if (F_.isSet(Flag::NEGATIVE))
            {
                shift += (0x06 * F_.isSet(Flag::HALF_CARRY));
                shift += (0x60 * F_.isSet(Flag::CARRY));

                extendedResult = (A() - shift);
            }
            else
            {
                shift += (0x06 * (F_.isSet(Flag::HALF_CARRY) || ((A() & 0x0F) > 0x09)));

                if (F_.isSet(Flag::CARRY) || (A() > 0x99))
                {
                    shift += 0x60;
                    F_.setFlag(Flag::CARRY, true);
                }

                extendedResult = (A() + shift);
            }

            std::uint8_t const result = getLower(extendedResult);

            A_ = result;
            F_.setFlag(Flag::ZERO, result == 0);
            F_.setFlag(Flag::HALF_CARRY, false);
            break;
        }
        case 0x28:
        {
            JR(F_.isSet(Flag::ZERO));
            break;
        }
        case 0x29:
        {
            ADD(HL_, HL());
            break;
        }
        case 0x2A:
        {
            auto const address        = Address(HL());
            std::uint16_t const value = (HL() + 1);

            HL_.lower() = getLower(value);
            HL_.upper() = getUpper(value);
            A_          = read(address);
            break;
        }
        case 0x2B:
        {
            DEC(HL_);
            break;
        }
        case 0x2C:
        {
            INC(L_);
            break;
        }
        case 0x2D:
        {
            DEC(L_);
            break;
        }
        case 0x2E:
        {
            L_ = readNextByteAndAdvance();
            break;
        }
        case 0x2F:
        {
            A_ = ~A();
            F_.setFlag(Flag::NEGATIVE, true);
            F_.setFlag(Flag::HALF_CARRY, true);
            break;
        }
        case 0x30:
        {
            JR(!F_.isSet(Flag::CARRY));
            break;
        }
        case 0x31:
        {
            SP_.setLower(readNextByteAndAdvance());
            SP_.setUpper(readNextByteAndAdvance());
            break;
        }
        case 0x32:
        {
            auto const address        = Address(HL());
            std::uint16_t const value = (HL() - 1);

            HL_.lower() = getLower(value);
            HL_.upper() = getUpper(value);
            write(address, A());
            break;
        }
        case 0x33:
        {
            INC(SP_);
            break;
        }
        case 0x34:
        {
            auto const address = Address(HL());
            auto reg           = ByteRegister(read(address));
            INC(reg);
            write(address, reg());
            break;
        }
        case 0x35:
        {
            auto const address = Address(HL());
            auto reg           = ByteRegister(read(address));
            DEC(reg);
            write(address, reg());
            break;
        }
        case 0x36:
        {
            std::uint8_t const value = readNextByteAndAdvance();
            write(Address(HL()), value);
            break;
        }
        case 0x37:
        {
            F_.setFlag(Flag::NEGATIVE, false);
            F_.setFlag(Flag::HALF_CARRY, false);
            F_.setFlag(Flag::CARRY, true);
            break;
        }
        case 0x38:
        {
            JR(F_.isSet(Flag::CARRY));
            break;
        }
        case 0x39:
        {
            ADD(HL_, SP());
            break;
        }
        case 0x3A:
        {
            auto const address        = Address(HL());
            std::uint16_t const value = (HL() - 1);

            HL_.lower() = getLower(value);
            HL_.upper() = getUpper(value);
            A_          = read(address);
            break;
        }
        case 0x3B:
        {
            DEC(SP_);
            break;
        }
        case 0x3C:
        {
            INC(A_);
            break;
        }
        case 0x3D:
        {
            DEC(A_);
            break;
        }
        case 0x3E:
        {
            A_ = readNextByteAndAdvance();
            break;
        }
        case 0x3F:
        {
            F_.setFlag(Flag::NEGATIVE, false);
            F_.setFlag(Flag::HALF_CARRY, false);
            F_.toggleFlag(Flag::CARRY);
            break;
        }
        case 0x40:
        {
            B_ = B();
            break;
        }
        case 0x41:
        {
            B_ = C();
            break;
        }
        case 0x42:
        {
            B_ = D();
            break;
        }
        case 0x43:
        {
            B_ = E();
            break;
        }
        case 0x44:
        {
            B_ = H();
            break;
        }
        case 0x45:
        {
            B_ = L();
            break;
        }
        case 0x46:
        {
            B_ = read(Address(HL()));
            break;
        }
        case 0x47:
        {
            B_ = A();
            break;
        }
        case 0x48:
        {
            C_ = B();
            break;
        }
        case 0x49:
        {
            C_ = C();
            break;
        }
        case 0x4A:
        {
            C_ = D();
            break;
        }
        case 0x4B:
        {
            C_ = E();
            break;
        }
        case 0x4C:
        {
            C_ = H();
            break;
        }
        case 0x4D:
        {
            C_ = L();
            break;
        }
        case 0x4E:
        {
            C_ = read(Address(HL()));
            break;
        }
        case 0x4F:
        {
            C_ = A();
            break;
        }
        case 0x50:
        {
            D_ = B();
            break;
        }
        case 0x51:
        {
            D_ = C();
            break;
        }
        case 0x52:
        {
            D_ = D();
            break;
        }
        case 0x53:
        {
            D_ = E();
            break;
        }
        case 0x54:
        {
            D_ = H();
            break;
        }
        case 0x55:
        {
            D_ = L();
            break;
        }
        case 0x56:
        {
            D_ = read(Address(HL()));
            break;
        }
        case 0x57:
        {
            D_ = A();
            break;
        }
        case 0x58:
        {
            E_ = B();
            break;
        }
        case 0x59:
        {
            E_ = C();
            break;
        }
        case 0x5A:
        {
            E_ = D();
            break;
        }
        case 0x5B:
        {
            E_ = E();
            break;
        }
        case 0x5C:
        {
            E_ = H();
            break;
        }
        case 0x5D:
        {
            E_ = L();
            break;
        }
        case 0x5E:
        {
            E_ = read(Address(HL()));
            break;
        }
        case 0x5F:
        {
            E_ = A();
            break;
        }
        case 0x60:
        {
            H_ = B();
            break;
        }
        case 0x61:
        {
            H_ = C();
            break;
        }
        case 0x62:
        {
            H_ = D();
            break;
        }
        case 0x63:
        {
            H_ = E();
            break;
        }
        case 0x64:
        {
            H_ = H();
            break;
        }
        case 0x65:
        {
            H_ = L();
            break;
        }
        case 0x66:
        {
            H_ = read(Address(HL()));
            break;
        }
        case 0x67:
        {
            H_ = A();
            break;
        }
        case 0x68:
        {
            L_ = B();
            break;
        }
        case 0x69:
        {
            L_ = C();
            break;
        }
        case 0x6A:
        {
            L_ = D();
            break;
        }
        case 0x6B:
        {
            L_ = E();
            break;
        }
        case 0x6C:
        {
            L_ = H();
            break;
        }
        case 0x6D:
        {
            L_ = L();
            break;
        }
        case 0x6E:
        {
            L_ = read(Address(HL()));
            break;
        }
        case 0x6F:
        {
            L_ = A();
            break;
        }
        case 0x70:
        {
            write(Address(HL()), B());
            break;
        }
        case 0x71:
        {
            write(Address(HL()), C());
            break;
        }
        case 0x72:
        {
            write(Address(HL()), D());
            break;
        }
        case 0x73:
        {
            write(Address(HL()), E());
            break;
        }
        case 0x74:
        {
            write(Address(HL()), H());
            break;
        }
        case 0x75:
        {
            write(Address(HL()), L());
            break;
        }
        case 0x76:
        {
            // TODO: Implement HALT
            // SingleStepTests expects 3 m-cycles here while the gbops table lists it as a 1 m-cycle instruction
            // follow the test suite for now
            tick();
            tick();
            break;
        }
        case 0x77:
        {
            write(Address(HL()), A());
            break;
        }
        case 0x78:
        {
            A_ = B();
            break;
        }
        case 0x79:
        {
            A_ = C();
            break;
        }
        case 0x7A:
        {
            A_ = D();
            break;
        }
        case 0x7B:
        {
            A_ = E();
            break;
        }
        case 0x7C:
        {
            A_ = H();
            break;
        }
        case 0x7D:
        {
            A_ = L();
            break;
        }
        case 0x7E:
        {
            A_ = read(Address(HL()));
            break;
        }
        case 0x7F:
        {
            A_ = A();
            break;
        }
        case 0x80:
        {
            ADD(A_, B());
            break;
        }
        case 0x81:
        {
            ADD(A_, C());
            break;
        }
        case 0x82:
        {
            ADD(A_, D());
            break;
        }
        case 0x83:
        {
            ADD(A_, E());
            break;
        }
        case 0x84:
        {
            ADD(A_, H());
            break;
        }
        case 0x85:
        {
            ADD(A_, L());
            break;
        }
        case 0x86:
        {
            std::uint8_t const value = read(Address(HL()));
            ADD(A_, value);
            break;
        }
        case 0x87:
        {
            ADD(A_, A());
            break;
        }
        case 0x88:
        {
            ADC(A_, B());
            break;
        }
        case 0x89:
        {
            ADC(A_, C());
            break;
        }
        case 0x8A:
        {
            ADC(A_, D());
            break;
        }
        case 0x8B:
        {
            ADC(A_, E());
            break;
        }
        case 0x8C:
        {
            ADC(A_, H());
            break;
        }
        case 0x8D:
        {
            ADC(A_, L());
            break;
        }
        case 0x8E:
        {
            std::uint8_t const value = read(Address(HL()));
            ADC(A_, value);
            break;
        }
        case 0x8F:
        {
            ADC(A_, A());
            break;
        }
        case 0x90:
        {
            SUB(A_, B());
            break;
        }
        case 0x91:
        {
            SUB(A_, C());
            break;
        }
        case 0x92:
        {
            SUB(A_, D());
            break;
        }
        case 0x93:
        {
            SUB(A_, E());
            break;
        }
        case 0x94:
        {
            SUB(A_, H());
            break;
        }
        case 0x95:
        {
            SUB(A_, L());
            break;
        }
        case 0x96:
        {
            std::uint8_t const value = read(Address(HL()));
            SUB(A_, value);
            break;
        }
        case 0x97:
        {
            SUB(A_, A());
            break;
        }
        case 0x98:
        {
            SBC(A_, B());
            break;
        }
        case 0x99:
        {
            SBC(A_, C());
            break;
        }
        case 0x9A:
        {
            SBC(A_, D());
            break;
        }
        case 0x9B:
        {
            SBC(A_, E());
            break;
        }
        case 0x9C:
        {
            SBC(A_, H());
            break;
        }
        case 0x9D:
        {
            SBC(A_, L());
            break;
        }
        case 0x9E:
        {
            std::uint8_t const value = read(Address(HL()));
            SBC(A_, value);
            break;
        }
        case 0x9F:
        {
            SBC(A_, A());
            break;
        }
        case 0xA0:
        {
            AND(A_, B());
            break;
        }
        case 0xA1:
        {
            AND(A_, C());
            break;
        }
        case 0xA2:
        {
            AND(A_, D());
            break;
        }
        case 0xA3:
        {
            AND(A_, E());
            break;
        }
        case 0xA4:
        {
            AND(A_, H());
            break;
        }
        case 0xA5:
        {
            AND(A_, L());
            break;
        }
        case 0xA6:
        {
            std::uint8_t const value = read(Address(HL()));
            AND(A_, value);
            break;
        }
        case 0xA7:
        {
            AND(A_, A());
            break;
        }
        case 0xA8:
        {
            XOR(A_, B());
            break;
        }
        case 0xA9:
        {
            XOR(A_, C());
            break;
        }
        case 0xAA:
        {
            XOR(A_, D());
            break;
        }
        case 0xAB:
        {
            XOR(A_, E());
            break;
        }
        case 0xAC:
        {
            XOR(A_, H());
            break;
        }
        case 0xAD:
        {
            XOR(A_, L());
            break;
        }
        case 0xAE:
        {
            std::uint8_t const value = read(Address(HL()));
            XOR(A_, value);
            break;
        }
        case 0xAF:
        {
            XOR(A_, A());
            break;
        }
        case 0xB0:
        {
            OR(A_, B());
            break;
        }
        case 0xB1:
        {
            OR(A_, C());
            break;
        }
        case 0xB2:
        {
            OR(A_, D());
            break;
        }
        case 0xB3:
        {
            OR(A_, E());
            break;
        }
        case 0xB4:
        {
            OR(A_, H());
            break;
        }
        case 0xB5:
        {
            OR(A_, L());
            break;
        }
        case 0xB6:
        {
            std::uint8_t const value = read(Address(HL()));
            OR(A_, value);
            break;
        }
        case 0xB7:
        {
            OR(A_, A());
            break;
        }
        case 0xB8:
        {
            CP(A_, B());
            break;
        }
        case 0xB9:
        {
            CP(A_, C());
            break;
        }
        case 0xBA:
        {
            CP(A_, D());
            break;
        }
        case 0xBB:
        {
            CP(A_, E());
            break;
        }
        case 0xBC:
        {
            CP(A_, H());
            break;
        }
        case 0xBD:
        {
            CP(A_, L());
            break;
        }
        case 0xBE:
        {
            std::uint8_t const value = read(Address(HL()));
            CP(A_, value);
            break;
        }
        case 0xBF:
        {
            CP(A_, A());
            break;
        }
        case 0xC0:
        {
            RET(!F_.isSet(Flag::ZERO));
            break;
        }
        case 0xC1:
        {
            POP(BC_);
            break;
        }
        case 0xC2:
        {
            JP(!F_.isSet(Flag::ZERO));
            break;
        }
        case 0xC3:
        {
            JP();
            break;
        }
        case 0xC4:
        {
            CALL(!F_.isSet(Flag::ZERO));
            break;
        }
        case 0xC5:
        {
            PUSH(BC_);
            break;
        }
        case 0xC6:
        {
            std::uint8_t const value = readNextByteAndAdvance();
            ADD(A_, value);
            break;
        }
        case 0xC7:
        {
            RST(0x00);
            break;
        }
        case 0xC8:
        {
            RET(F_.isSet(Flag::ZERO));
            break;
        }
        case 0xC9:
        {
            RET();
            break;
        }
        case 0xCA:
        {
            JP(F_.isSet(Flag::ZERO));
            break;
        }
        case 0xCB:
        {
            std::uint8_t const prefixPart      = opcode;
            std::uint8_t const offsetPart      = readNextByteAndAdvance();
            std::uint16_t const extendedOpcode = ((prefixPart << 8) | offsetPart);
            executeExtended(extendedOpcode);
            break;
        }
        case 0xCC:
        {
            CALL(F_.isSet(Flag::ZERO));
            break;
        }
        case 0xCD:
        {
            CALL();
            break;
        }
        case 0xCE:
        {
            std::uint8_t const value = readNextByteAndAdvance();
            ADC(A_, value);
            break;
        }
        case 0xCF:
        {
            RST(0x08);
            break;
        }
        case 0xD0:
        {
            RET(!F_.isSet(Flag::CARRY));
            break;
        }
        case 0xD1:
        {
            POP(DE_);
            break;
        }
        case 0xD2:
        {
            JP(!F_.isSet(Flag::CARRY));
            break;
        }
        case 0xD3:
        {
            throw IllegalOpcodeException(opcode);
        }
        case 0xD4:
        {
            CALL(!F_.isSet(Flag::CARRY));
            break;
        }
        case 0xD5:
        {
            PUSH(DE_);
            break;
        }
        case 0xD6:
        {
            std::uint8_t const value = readNextByteAndAdvance();
            SUB(A_, value);
            break;
        }
        case 0xD7:
        {
            RST(0x10);
            break;
        }
        case 0xD8:
        {
            RET(F_.isSet(Flag::CARRY));
            break;
        }
        case 0xD9:
        {
            // TODO: Implement interrupt part of RETI
            RET();
            break;
        }
        case 0xDA:
        {
            JP(F_.isSet(Flag::CARRY));
            break;
        }
        case 0xDB:
        {
            throw IllegalOpcodeException(opcode);
        }
        case 0xDC:
        {
            CALL(F_.isSet(Flag::CARRY));
            break;
        }
        case 0xDD:
        {
            throw IllegalOpcodeException(opcode);
        }
        case 0xDE:
        {
            std::uint8_t const value = readNextByteAndAdvance();
            SBC(A_, value);
            break;
        }
        case 0xDF:
        {
            RST(0x18);
            break;
        }
        case 0xE0:
        {
            std::uint8_t const offset = readNextByteAndAdvance();
            write(Address(0xFF00 + offset), A());
            break;
        }
        case 0xE1:
        {
            POP(HL_);
            break;
        }
        case 0xE2:
        {
            write(Address(0xFF00 + C()), A());
            break;
        }
        case 0xE3:
        {
            throw IllegalOpcodeException(opcode);
        }
        case 0xE4:
        {
            throw IllegalOpcodeException(opcode);
        }
        case 0xE5:
        {
            PUSH(HL_);
            break;
        }
        case 0xE6:
        {
            std::uint8_t const value = readNextByteAndAdvance();
            AND(A_, value);
            break;
        }
        case 0xE7:
        {
            RST(0x20);
            break;
        }
        case 0xE8:
        {
            auto const shift                  = static_cast<std::int8_t>(readNextByteAndAdvance());
            std::uint16_t const oldValue      = SP();
            std::int32_t const extendedResult = (oldValue + shift);
            std::uint16_t const result        = (extendedResult & 0xFFFF);

            tick();
            SP_.setLower(result & 0xFF);
            // SingleStepTests treats this as an internal cycle while the gbops table lists it as a write cycle
            // follow the test suite for now
            tick();
            SP_.setUpper((result >> 8) & 0xFF);
            F_.setFlag(Flag::ZERO, false);
            F_.setFlag(Flag::NEGATIVE, false);
            // The carry detection on signed arithmetic is taken from gbemu
            F_.setFlag(Flag::HALF_CARRY, ((oldValue ^ shift ^ (extendedResult & 0xFFFF)) & 0x10) == 0x10);
            F_.setFlag(Flag::CARRY, ((oldValue ^ shift ^ (extendedResult & 0xFFFF)) & 0x100) == 0x100);
            break;
        }
        case 0xE9:
        {
            PC_ = HL();
            break;
        }
        case 0xEA:
        {
            std::uint8_t const lo = readNextByteAndAdvance();
            std::uint8_t const hi = readNextByteAndAdvance();
            auto const address    = Address((hi << 8) | lo);
            write(address, A());
            break;
        }
        case 0xEB:
        {
            throw IllegalOpcodeException(opcode);
        }
        case 0xEC:
        {
            throw IllegalOpcodeException(opcode);
        }
        case 0xED:
        {
            throw IllegalOpcodeException(opcode);
        }
        case 0xEE:
        {
            std::uint8_t const value = readNextByteAndAdvance();
            XOR(A_, value);
            break;
        }
        case 0xEF:
        {
            RST(0x28);
            break;
        }
        case 0xF0:
        {
            std::uint8_t const offset = readNextByteAndAdvance();

            A_ = read(Address(0xFF00 + offset));
            break;
        }
        case 0xF1:
        {
            auto const addrLo = Address(SP());
            ++SP_;
            std::uint8_t const lo = read(addrLo);
            AF_.lower()           = (lo & 0xF0);

            auto const addrHi = Address(SP());
            ++SP_;
            AF_.upper() = read(addrHi);
            break;
        }
        case 0xF2:
        {
            A_ = read(Address(0xFF00 + C()));
            break;
        }
        case 0xF3:
        {
            // TODO: Implement DI
            break;
        }
        case 0xF4:
        {
            throw IllegalOpcodeException(opcode);
        }
        case 0xF5:
        {
            PUSH(AF_);
            break;
        }
        case 0xF6:
        {
            std::uint8_t const value = readNextByteAndAdvance();
            OR(A_, value);
            break;
        }
        case 0xF7:
        {
            RST(0x30);
            break;
        }
        case 0xF8:
        {
            auto const shift                  = static_cast<std::int8_t>(readNextByteAndAdvance());
            std::uint16_t const oldValue      = SP();
            std::int32_t const extendedResult = (oldValue + shift);
            std::uint16_t const result        = (extendedResult & 0xFFFF);

            HL_.lower() = (result & 0xFF);
            tick();
            HL_.upper() = ((result >> 8) & 0xFF);
            F_.setFlag(Flag::ZERO, false);
            F_.setFlag(Flag::NEGATIVE, false);
            // The carry detection on signed arithmetic is taken from gbemu
            F_.setFlag(Flag::HALF_CARRY, ((oldValue ^ shift ^ (extendedResult & 0xFFFF)) & 0x10) == 0x10);
            F_.setFlag(Flag::CARRY, ((oldValue ^ shift ^ (extendedResult & 0xFFFF)) & 0x100) == 0x100);
            break;
        }
        case 0xF9:
        {
            tick();
            SP_ = HL();
            break;
        }
        case 0xFA:
        {
            std::uint8_t const lo = readNextByteAndAdvance();
            std::uint8_t const hi = readNextByteAndAdvance();
            auto const address    = Address((hi << 8) | lo);

            A_ = read(address);
            break;
        }
        case 0xFB:
        {
            // TODO: Implement EI
            break;
        }
        case 0xFC:
        {
            throw IllegalOpcodeException(opcode);
        }
        case 0xFD:
        {
            throw IllegalOpcodeException(opcode);
        }
        case 0xFE:
        {
            std::uint8_t const value = readNextByteAndAdvance();
            CP(A_, value);
            break;
        }
        case 0xFF:
        {
            RST(0x38);
            break;
        }
        default: throw OpcodeNotImplementedException(opcode);
    }
}

void Cpu::executeExtended(std::uint16_t opcode)
{
    switch (opcode)
    {
        case 0xCB00:
        {
            RLC(B_);
            break;
        }
        case 0xCB01:
        {
            RLC(C_);
            break;
        }
        case 0xCB02:
        {
            RLC(D_);
            break;
        }
        case 0xCB03:
        {
            RLC(E_);
            break;
        }
        case 0xCB04:
        {
            RLC(H_);
            break;
        }
        case 0xCB05:
        {
            RLC(L_);
            break;
        }
        case 0xCB06:
        {
            RLC(Address(HL()));
            break;
        }
        case 0xCB07:
        {
            RLC(A_);
            break;
        }
        case 0xCB08:
        {
            RRC(B_);
            break;
        }
        case 0xCB09:
        {
            RRC(C_);
            break;
        }
        case 0xCB0A:
        {
            RRC(D_);
            break;
        }
        case 0xCB0B:
        {
            RRC(E_);
            break;
        }
        case 0xCB0C:
        {
            RRC(H_);
            break;
        }
        case 0xCB0D:
        {
            RRC(L_);
            break;
        }
        case 0xCB0E:
        {
            RRC(Address(HL()));
            break;
        }
        case 0xCB0F:
        {
            RRC(A_);
            break;
        }
        case 0xCB10:
        {
            RL(B_);
            break;
        }
        case 0xCB11:
        {
            RL(C_);
            break;
        }
        case 0xCB12:
        {
            RL(D_);
            break;
        }
        case 0xCB13:
        {
            RL(E_);
            break;
        }
        case 0xCB14:
        {
            RL(H_);
            break;
        }
        case 0xCB15:
        {
            RL(L_);
            break;
        }
        case 0xCB16:
        {
            RL(Address(HL()));
            break;
        }
        case 0xCB17:
        {
            RL(A_);
            break;
        }
        case 0xCB18:
        {
            RR(B_);
            break;
        }
        case 0xCB19:
        {
            RR(C_);
            break;
        }
        case 0xCB1A:
        {
            RR(D_);
            break;
        }
        case 0xCB1B:
        {
            RR(E_);
            break;
        }
        case 0xCB1C:
        {
            RR(H_);
            break;
        }
        case 0xCB1D:
        {
            RR(L_);
            break;
        }
        case 0xCB1E:
        {
            RR(Address(HL()));
            break;
        }
        case 0xCB1F:
        {
            RR(A_);
            break;
        }
        case 0xCB20:
        {
            SLA(B_);
            break;
        }
        case 0xCB21:
        {
            SLA(C_);
            break;
        }
        case 0xCB22:
        {
            SLA(D_);
            break;
        }
        case 0xCB23:
        {
            SLA(E_);
            break;
        }
        case 0xCB24:
        {
            SLA(H_);
            break;
        }
        case 0xCB25:
        {
            SLA(L_);
            break;
        }
        case 0xCB26:
        {
            SLA(Address(HL()));
            break;
        }
        case 0xCB27:
        {
            SLA(A_);
            break;
        }
        case 0xCB28:
        {
            SRA(B_);
            break;
        }
        case 0xCB29:
        {
            SRA(C_);
            break;
        }
        case 0xCB2A:
        {
            SRA(D_);
            break;
        }
        case 0xCB2B:
        {
            SRA(E_);
            break;
        }
        case 0xCB2C:
        {
            SRA(H_);
            break;
        }
        case 0xCB2D:
        {
            SRA(L_);
            break;
        }
        case 0xCB2E:
        {
            SRA(Address(HL()));
            break;
        }
        case 0xCB2F:
        {
            SRA(A_);
            break;
        }
        case 0xCB30:
        {
            SWAP(B_);
            break;
        }
        case 0xCB31:
        {
            SWAP(C_);
            break;
        }
        case 0xCB32:
        {
            SWAP(D_);
            break;
        }
        case 0xCB33:
        {
            SWAP(E_);
            break;
        }
        case 0xCB34:
        {
            SWAP(H_);
            break;
        }
        case 0xCB35:
        {
            SWAP(L_);
            break;
        }
        case 0xCB36:
        {
            SWAP(Address(HL()));
            break;
        }
        case 0xCB37:
        {
            SWAP(A_);
            break;
        }
        case 0xCB38:
        {
            SRL(B_);
            break;
        }
        case 0xCB39:
        {
            SRL(C_);
            break;
        }
        case 0xCB3A:
        {
            SRL(D_);
            break;
        }
        case 0xCB3B:
        {
            SRL(E_);
            break;
        }
        case 0xCB3C:
        {
            SRL(H_);
            break;
        }
        case 0xCB3D:
        {
            SRL(L_);
            break;
        }
        case 0xCB3E:
        {
            SRL(Address(HL()));
            break;
        }
        case 0xCB3F:
        {
            SRL(A_);
            break;
        }
        case 0xCB40:
        {
            BIT(0, B_);
            break;
        }
        case 0xCB41:
        {
            BIT(0, C_);
            break;
        }
        case 0xCB42:
        {
            BIT(0, D_);
            break;
        }
        case 0xCB43:
        {
            BIT(0, E_);
            break;
        }
        case 0xCB44:
        {
            BIT(0, H_);
            break;
        }
        case 0xCB45:
        {
            BIT(0, L_);
            break;
        }
        case 0xCB46:
        {
            BIT(0, Address(HL()));
            break;
        }
        case 0xCB47:
        {
            BIT(0, A_);
            break;
        }
        case 0xCB48:
        {
            BIT(1, B_);
            break;
        }
        case 0xCB49:
        {
            BIT(1, C_);
            break;
        }
        case 0xCB4A:
        {
            BIT(1, D_);
            break;
        }
        case 0xCB4B:
        {
            BIT(1, E_);
            break;
        }
        case 0xCB4C:
        {
            BIT(1, H_);
            break;
        }
        case 0xCB4D:
        {
            BIT(1, L_);
            break;
        }
        case 0xCB4E:
        {
            BIT(1, Address(HL()));
            break;
        }
        case 0xCB4F:
        {
            BIT(1, A_);
            break;
        }
        case 0xCB50:
        {
            BIT(2, B_);
            break;
        }
        case 0xCB51:
        {
            BIT(2, C_);
            break;
        }
        case 0xCB52:
        {
            BIT(2, D_);
            break;
        }
        case 0xCB53:
        {
            BIT(2, E_);
            break;
        }
        case 0xCB54:
        {
            BIT(2, H_);
            break;
        }
        case 0xCB55:
        {
            BIT(2, L_);
            break;
        }
        case 0xCB56:
        {
            BIT(2, Address(HL()));
            break;
        }
        case 0xCB57:
        {
            BIT(2, A_);
            break;
        }
        case 0xCB58:
        {
            BIT(3, B_);
            break;
        }
        case 0xCB59:
        {
            BIT(3, C_);
            break;
        }
        case 0xCB5A:
        {
            BIT(3, D_);
            break;
        }
        case 0xCB5B:
        {
            BIT(3, E_);
            break;
        }
        case 0xCB5C:
        {
            BIT(3, H_);
            break;
        }
        case 0xCB5D:
        {
            BIT(3, L_);
            break;
        }
        case 0xCB5E:
        {
            BIT(3, Address(HL()));
            break;
        }
        case 0xCB5F:
        {
            BIT(3, A_);
            break;
        }
        case 0xCB60:
        {
            BIT(4, B_);
            break;
        }
        case 0xCB61:
        {
            BIT(4, C_);
            break;
        }
        case 0xCB62:
        {
            BIT(4, D_);
            break;
        }
        case 0xCB63:
        {
            BIT(4, E_);
            break;
        }
        case 0xCB64:
        {
            BIT(4, H_);
            break;
        }
        case 0xCB65:
        {
            BIT(4, L_);
            break;
        }
        case 0xCB66:
        {
            BIT(4, Address(HL()));
            break;
        }
        case 0xCB67:
        {
            BIT(4, A_);
            break;
        }
        case 0xCB68:
        {
            BIT(5, B_);
            break;
        }
        case 0xCB69:
        {
            BIT(5, C_);
            break;
        }
        case 0xCB6A:
        {
            BIT(5, D_);
            break;
        }
        case 0xCB6B:
        {
            BIT(5, E_);
            break;
        }
        case 0xCB6C:
        {
            BIT(5, H_);
            break;
        }
        case 0xCB6D:
        {
            BIT(5, L_);
            break;
        }
        case 0xCB6E:
        {
            BIT(5, Address(HL()));
            break;
        }
        case 0xCB6F:
        {
            BIT(5, A_);
            break;
        }
        case 0xCB70:
        {
            BIT(6, B_);
            break;
        }
        case 0xCB71:
        {
            BIT(6, C_);
            break;
        }
        case 0xCB72:
        {
            BIT(6, D_);
            break;
        }
        case 0xCB73:
        {
            BIT(6, E_);
            break;
        }
        case 0xCB74:
        {
            BIT(6, H_);
            break;
        }
        case 0xCB75:
        {
            BIT(6, L_);
            break;
        }
        case 0xCB76:
        {
            BIT(6, Address(HL()));
            break;
        }
        case 0xCB77:
        {
            BIT(6, A_);
            break;
        }
        case 0xCB78:
        {
            BIT(7, B_);
            break;
        }
        case 0xCB79:
        {
            BIT(7, C_);
            break;
        }
        case 0xCB7A:
        {
            BIT(7, D_);
            break;
        }
        case 0xCB7B:
        {
            BIT(7, E_);
            break;
        }
        case 0xCB7C:
        {
            BIT(7, H_);
            break;
        }
        case 0xCB7D:
        {
            BIT(7, L_);
            break;
        }
        case 0xCB7E:
        {
            BIT(7, Address(HL()));
            break;
        }
        case 0xCB7F:
        {
            BIT(7, A_);
            break;
        }
        case 0xCB80:
        {
            RES(0, B_);
            break;
        }
        case 0xCB81:
        {
            RES(0, C_);
            break;
        }
        case 0xCB82:
        {
            RES(0, D_);
            break;
        }
        case 0xCB83:
        {
            RES(0, E_);
            break;
        }
        case 0xCB84:
        {
            RES(0, H_);
            break;
        }
        case 0xCB85:
        {
            RES(0, L_);
            break;
        }
        case 0xCB86:
        {
            RES(0, Address(HL()));
            break;
        }
        case 0xCB87:
        {
            RES(0, A_);
            break;
        }
        case 0xCB88:
        {
            RES(1, B_);
            break;
        }
        case 0xCB89:
        {
            RES(1, C_);
            break;
        }
        case 0xCB8A:
        {
            RES(1, D_);
            break;
        }
        case 0xCB8B:
        {
            RES(1, E_);
            break;
        }
        case 0xCB8C:
        {
            RES(1, H_);
            break;
        }
        case 0xCB8D:
        {
            RES(1, L_);
            break;
        }
        case 0xCB8E:
        {
            RES(1, Address(HL()));
            break;
        }
        case 0xCB8F:
        {
            RES(1, A_);
            break;
        }
        case 0xCB90:
        {
            RES(2, B_);
            break;
        }
        case 0xCB91:
        {
            RES(2, C_);
            break;
        }
        case 0xCB92:
        {
            RES(2, D_);
            break;
        }
        case 0xCB93:
        {
            RES(2, E_);
            break;
        }
        case 0xCB94:
        {
            RES(2, H_);
            break;
        }
        case 0xCB95:
        {
            RES(2, L_);
            break;
        }
        case 0xCB96:
        {
            RES(2, Address(HL()));
            break;
        }
        case 0xCB97:
        {
            RES(2, A_);
            break;
        }
        case 0xCB98:
        {
            RES(3, B_);
            break;
        }
        case 0xCB99:
        {
            RES(3, C_);
            break;
        }
        case 0xCB9A:
        {
            RES(3, D_);
            break;
        }
        case 0xCB9B:
        {
            RES(3, E_);
            break;
        }
        case 0xCB9C:
        {
            RES(3, H_);
            break;
        }
        case 0xCB9D:
        {
            RES(3, L_);
            break;
        }
        case 0xCB9E:
        {
            RES(3, Address(HL()));
            break;
        }
        case 0xCB9F:
        {
            RES(3, A_);
            break;
        }
        case 0xCBA0:
        {
            RES(4, B_);
            break;
        }
        case 0xCBA1:
        {
            RES(4, C_);
            break;
        }
        case 0xCBA2:
        {
            RES(4, D_);
            break;
        }
        case 0xCBA3:
        {
            RES(4, E_);
            break;
        }
        case 0xCBA4:
        {
            RES(4, H_);
            break;
        }
        case 0xCBA5:
        {
            RES(4, L_);
            break;
        }
        case 0xCBA6:
        {
            RES(4, Address(HL()));
            break;
        }
        case 0xCBA7:
        {
            RES(4, A_);
            break;
        }
        case 0xCBA8:
        {
            RES(5, B_);
            break;
        }
        case 0xCBA9:
        {
            RES(5, C_);
            break;
        }
        case 0xCBAA:
        {
            RES(5, D_);
            break;
        }
        case 0xCBAB:
        {
            RES(5, E_);
            break;
        }
        case 0xCBAC:
        {
            RES(5, H_);
            break;
        }
        case 0xCBAD:
        {
            RES(5, L_);
            break;
        }
        case 0xCBAE:
        {
            RES(5, Address(HL()));
            break;
        }
        case 0xCBAF:
        {
            RES(5, A_);
            break;
        }
        case 0xCBB0:
        {
            RES(6, B_);
            break;
        }
        case 0xCBB1:
        {
            RES(6, C_);
            break;
        }
        case 0xCBB2:
        {
            RES(6, D_);
            break;
        }
        case 0xCBB3:
        {
            RES(6, E_);
            break;
        }
        case 0xCBB4:
        {
            RES(6, H_);
            break;
        }
        case 0xCBB5:
        {
            RES(6, L_);
            break;
        }
        case 0xCBB6:
        {
            RES(6, Address(HL()));
            break;
        }
        case 0xCBB7:
        {
            RES(6, A_);
            break;
        }
        case 0xCBB8:
        {
            RES(7, B_);
            break;
        }
        case 0xCBB9:
        {
            RES(7, C_);
            break;
        }
        case 0xCBBA:
        {
            RES(7, D_);
            break;
        }
        case 0xCBBB:
        {
            RES(7, E_);
            break;
        }
        case 0xCBBC:
        {
            RES(7, H_);
            break;
        }
        case 0xCBBD:
        {
            RES(7, L_);
            break;
        }
        case 0xCBBE:
        {
            RES(7, Address(HL()));
            break;
        }
        case 0xCBBF:
        {
            RES(7, A_);
            break;
        }
        case 0xCBC0:
        {
            SET(0, B_);
            break;
        }
        case 0xCBC1:
        {
            SET(0, C_);
            break;
        }
        case 0xCBC2:
        {
            SET(0, D_);
            break;
        }
        case 0xCBC3:
        {
            SET(0, E_);
            break;
        }
        case 0xCBC4:
        {
            SET(0, H_);
            break;
        }
        case 0xCBC5:
        {
            SET(0, L_);
            break;
        }
        case 0xCBC6:
        {
            SET(0, Address(HL()));
            break;
        }
        case 0xCBC7:
        {
            SET(0, A_);
            break;
        }
        case 0xCBC8:
        {
            SET(1, B_);
            break;
        }
        case 0xCBC9:
        {
            SET(1, C_);
            break;
        }
        case 0xCBCA:
        {
            SET(1, D_);
            break;
        }
        case 0xCBCB:
        {
            SET(1, E_);
            break;
        }
        case 0xCBCC:
        {
            SET(1, H_);
            break;
        }
        case 0xCBCD:
        {
            SET(1, L_);
            break;
        }
        case 0xCBCE:
        {
            SET(1, Address(HL()));
            break;
        }
        case 0xCBCF:
        {
            SET(1, A_);
            break;
        }
        case 0xCBD0:
        {
            SET(2, B_);
            break;
        }
        case 0xCBD1:
        {
            SET(2, C_);
            break;
        }
        case 0xCBD2:
        {
            SET(2, D_);
            break;
        }
        case 0xCBD3:
        {
            SET(2, E_);
            break;
        }
        case 0xCBD4:
        {
            SET(2, H_);
            break;
        }
        case 0xCBD5:
        {
            SET(2, L_);
            break;
        }
        case 0xCBD6:
        {
            SET(2, Address(HL()));
            break;
        }
        case 0xCBD7:
        {
            SET(2, A_);
            break;
        }
        case 0xCBD8:
        {
            SET(3, B_);
            break;
        }
        case 0xCBD9:
        {
            SET(3, C_);
            break;
        }
        case 0xCBDA:
        {
            SET(3, D_);
            break;
        }
        case 0xCBDB:
        {
            SET(3, E_);
            break;
        }
        case 0xCBDC:
        {
            SET(3, H_);
            break;
        }
        case 0xCBDD:
        {
            SET(3, L_);
            break;
        }
        case 0xCBDE:
        {
            SET(3, Address(HL()));
            break;
        }
        case 0xCBDF:
        {
            SET(3, A_);
            break;
        }
        case 0xCBE0:
        {
            SET(4, B_);
            break;
        }
        case 0xCBE1:
        {
            SET(4, C_);
            break;
        }
        case 0xCBE2:
        {
            SET(4, D_);
            break;
        }
        case 0xCBE3:
        {
            SET(4, E_);
            break;
        }
        case 0xCBE4:
        {
            SET(4, H_);
            break;
        }
        case 0xCBE5:
        {
            SET(4, L_);
            break;
        }
        case 0xCBE6:
        {
            SET(4, Address(HL()));
            break;
        }
        case 0xCBE7:
        {
            SET(4, A_);
            break;
        }
        case 0xCBE8:
        {
            SET(5, B_);
            break;
        }
        case 0xCBE9:
        {
            SET(5, C_);
            break;
        }
        case 0xCBEA:
        {
            SET(5, D_);
            break;
        }
        case 0xCBEB:
        {
            SET(5, E_);
            break;
        }
        case 0xCBEC:
        {
            SET(5, H_);
            break;
        }
        case 0xCBED:
        {
            SET(5, L_);
            break;
        }
        case 0xCBEE:
        {
            SET(5, Address(HL()));
            break;
        }
        case 0xCBEF:
        {
            SET(5, A_);
            break;
        }
        case 0xCBF0:
        {
            SET(6, B_);
            break;
        }
        case 0xCBF1:
        {
            SET(6, C_);
            break;
        }
        case 0xCBF2:
        {
            SET(6, D_);
            break;
        }
        case 0xCBF3:
        {
            SET(6, E_);
            break;
        }
        case 0xCBF4:
        {
            SET(6, H_);
            break;
        }
        case 0xCBF5:
        {
            SET(6, L_);
            break;
        }
        case 0xCBF6:
        {
            SET(6, Address(HL()));
            break;
        }
        case 0xCBF7:
        {
            SET(6, A_);
            break;
        }
        case 0xCBF8:
        {
            SET(7, B_);
            break;
        }
        case 0xCBF9:
        {
            SET(7, C_);
            break;
        }
        case 0xCBFA:
        {
            SET(7, D_);
            break;
        }
        case 0xCBFB:
        {
            SET(7, E_);
            break;
        }
        case 0xCBFC:
        {
            SET(7, H_);
            break;
        }
        case 0xCBFD:
        {
            SET(7, L_);
            break;
        }
        case 0xCBFE:
        {
            SET(7, Address(HL()));
            break;
        }
        case 0xCBFF:
        {
            SET(7, A_);
            break;
        }
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

void Cpu::INC(ByteRegister& reg) noexcept
{
    std::uint8_t const oldValue = reg();
    std::uint8_t const shift    = 1;
    std::uint8_t const result   = (oldValue + shift);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, (((oldValue & 0x0F) + shift) & 0x10) != 0);
}

void Cpu::INC(ShortRegister& reg) noexcept
{
    std::uint16_t const result = (reg() + 1);

    reg.setLower(getLower(result));
    tick();
    reg.setUpper(getUpper(result));
}

void Cpu::INC(RegisterPairView regPair) noexcept
{
    std::uint16_t const result = (regPair() + 1);

    regPair.lower() = getLower(result);
    tick();
    regPair.upper() = getUpper(result);
}

void Cpu::DEC(ByteRegister& reg) noexcept
{
    std::uint8_t const result = (reg() - 1);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, true);
    F_.setFlag(Flag::HALF_CARRY, (result & 0x0F) == 0x0F);
}

void Cpu::DEC(ShortRegister& reg) noexcept
{
    std::uint16_t const result = (reg() - 1);

    reg.setLower(getLower(result));
    tick();
    reg.setUpper(getUpper(result));
}

void Cpu::DEC(RegisterPairView regPair) noexcept
{
    std::uint16_t const result = (regPair() - 1);

    regPair.lower() = getLower(result);
    tick();
    regPair.upper() = getUpper(result);
}

void Cpu::ADD(ByteRegister& reg, std::uint8_t shift) noexcept
{
    std::uint8_t const oldValue        = reg();
    std::uint16_t const extendedResult = (oldValue + shift);
    std::uint8_t const result          = getLower(extendedResult);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, (((oldValue & 0x0F) + (shift & 0x0F)) & 0xF0) != 0);
    F_.setFlag(Flag::CARRY, (extendedResult & 0xFF00) != 0);
}

void Cpu::ADD(RegisterPairView regPair, std::uint16_t shift) noexcept
{
    std::uint16_t const oldValue = regPair();
    std::uint32_t const result   = (oldValue + shift);

    regPair.lower() = (result & 0xFF);
    tick();
    regPair.upper() = ((result >> 8) & 0xFF);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, (((oldValue & 0x0FFF) + (shift & 0x0FFF)) & 0x1000) != 0);
    F_.setFlag(Flag::CARRY, (result & 0x10000) != 0);
}

void Cpu::ADC(ByteRegister& reg, std::uint8_t shift) noexcept
{
    std::uint8_t const oldValue        = reg();
    std::uint8_t const carry           = (1 * F_.isSet(Flag::CARRY));
    std::uint16_t const extendedResult = (oldValue + shift + carry);
    std::uint8_t const result          = getLower(extendedResult);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, (((oldValue & 0x0F) + (shift & 0x0F) + carry) & 0xF0) != 0);
    F_.setFlag(Flag::CARRY, (extendedResult & 0xFF00) != 0);
}

void Cpu::SUB(ByteRegister& reg, std::uint8_t shift) noexcept
{
    std::uint8_t const oldValue = reg();
    std::uint8_t const result   = (oldValue - shift);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, true);
    F_.setFlag(Flag::HALF_CARRY, (shift & 0x0F) > (oldValue & 0x0F));
    F_.setFlag(Flag::CARRY, shift > oldValue);
}

void Cpu::SBC(ByteRegister& reg, std::uint8_t shift) noexcept
{
    std::uint8_t const oldValue = reg();
    std::uint8_t const carry    = (1 * F_.isSet(Flag::CARRY));
    std::uint8_t const result   = (oldValue - shift - carry);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, true);
    F_.setFlag(Flag::HALF_CARRY, ((shift & 0x0F) + carry) > (oldValue & 0x0F));
    F_.setFlag(Flag::CARRY, (shift + carry) > oldValue);
}

void Cpu::AND(ByteRegister& reg, std::uint8_t value) noexcept
{
    std::uint8_t const result = (reg() & value);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, true);
    F_.setFlag(Flag::CARRY, false);
}

void Cpu::XOR(ByteRegister& reg, std::uint8_t value) noexcept
{
    std::uint8_t const result = (reg() ^ value);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, false);
    F_.setFlag(Flag::CARRY, false);
}

void Cpu::OR(ByteRegister& reg, std::uint8_t value) noexcept
{
    std::uint8_t const result = (reg() | value);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, false);
    F_.setFlag(Flag::CARRY, false);
}

void Cpu::CP(ByteRegister const& reg, std::uint8_t shift) noexcept
{
    std::uint8_t const oldValue = reg();
    std::uint8_t const result   = (oldValue - shift);

    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, true);
    F_.setFlag(Flag::HALF_CARRY, (shift & 0x0F) > (oldValue & 0x0F));
    F_.setFlag(Flag::CARRY, shift > oldValue);
}

void Cpu::JR(bool shouldBranch) noexcept
{
    auto const shift = static_cast<std::int8_t>(readNextByteAndAdvance());
    if (shouldBranch)
    {
        tick();
        PC_ = (PC() + shift);
    }
}

void Cpu::JR() noexcept
{
    JR(true);
}

void Cpu::RET() noexcept
{
    auto const addrLo = Address(SP());
    ++SP_;
    std::uint8_t const lo = read(addrLo);

    auto const addrHi = Address(SP());
    ++SP_;
    std::uint8_t const hi = read(addrHi);

    tick();
    PC_ = ((hi << 8) | lo);
}

void Cpu::RET(bool shouldBranch) noexcept
{
    tick();
    if (shouldBranch)
    {
        RET();
    }
}

void Cpu::POP(RegisterPairView regPair) noexcept
{
    auto const addrLo = Address(SP());
    ++SP_;
    regPair.lower() = read(addrLo);

    auto const addrHi = Address(SP());
    ++SP_;
    regPair.upper() = read(addrHi);
}

void Cpu::JP(bool shouldBranch) noexcept
{
    std::uint8_t const lo = readNextByteAndAdvance();
    std::uint8_t const hi = readNextByteAndAdvance();

    if (shouldBranch)
    {
        tick();
        PC_ = ((hi << 8) | lo);
    }
}

void Cpu::JP() noexcept
{
    JP(true);
}

void Cpu::CALL(bool shouldBranch) noexcept
{
    std::uint8_t const lo     = readNextByteAndAdvance();
    std::uint8_t const hi     = readNextByteAndAdvance();
    std::uint16_t const value = ((hi << 8) | lo);

    if (shouldBranch)
    {
        tick();

        --SP_;
        auto const addrHi = Address(SP());
        write(addrHi, PC_.upper());

        --SP_;
        auto const addrLo = Address(SP());
        write(addrLo, PC_.lower());

        PC_ = value;
    }
}

void Cpu::CALL() noexcept
{
    CALL(true);
}

void Cpu::PUSH(RegisterPairView regPair) noexcept
{
    tick();

    --SP_;
    write(Address(SP()), regPair.upper()());

    --SP_;
    write(Address(SP()), regPair.lower()());
}

void Cpu::RST(std::uint8_t value) noexcept
{
    tick();

    --SP_;
    auto const addrHi = Address(SP());
    write(addrHi, PC_.upper());

    --SP_;
    auto const addrLo = Address(SP());
    write(addrLo, PC_.lower());

    PC_ = value;
}

void Cpu::RLC(ByteRegister& reg) noexcept
{
    std::uint8_t const value  = reg();
    std::uint8_t const carry  = (value & 0x80);
    std::uint8_t const result = ((value << 1) | (carry >> 7));

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, false);
    F_.setFlag(Flag::CARRY, carry != 0);
}

void Cpu::RLC(Address address) noexcept
{
    auto reg = ByteRegister(read(address));
    RLC(reg);
    write(address, reg());
}

void Cpu::RRC(ByteRegister& reg) noexcept
{
    std::uint8_t const value  = reg();
    std::uint8_t const carry  = (value & 0x01);
    std::uint8_t const result = ((value >> 1) | (carry << 7));

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, false);
    F_.setFlag(Flag::CARRY, carry != 0);
}

void Cpu::RRC(Address address) noexcept
{
    auto reg = ByteRegister(read(address));
    RRC(reg);
    write(address, reg());
}

void Cpu::RL(ByteRegister& reg) noexcept
{
    std::uint8_t const value  = reg();
    std::uint8_t const carry  = (value & 0x80);
    std::uint8_t const result = ((value << 1) | F_.isSet(Flag::CARRY));

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, false);
    F_.setFlag(Flag::CARRY, carry != 0);
}

void Cpu::RL(Address address) noexcept
{
    auto reg = ByteRegister(read(address));
    RL(reg);
    write(address, reg());
}

void Cpu::RR(ByteRegister& reg) noexcept
{
    std::uint8_t const value  = reg();
    std::uint8_t const carry  = (value & 0x01);
    std::uint8_t const result = ((value >> 1) | (F_.isSet(Flag::CARRY) << 7));

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, false);
    F_.setFlag(Flag::CARRY, carry != 0);
}

void Cpu::RR(Address address) noexcept
{
    auto reg = ByteRegister(read(address));
    RR(reg);
    write(address, reg());
}

void Cpu::SLA(ByteRegister& reg) noexcept
{
    std::uint8_t const value  = reg();
    std::uint8_t const carry  = (value & 0x80);
    std::uint8_t const result = (value << 1);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, false);
    F_.setFlag(Flag::CARRY, carry != 0);
}

void Cpu::SLA(Address address) noexcept
{
    auto reg = ByteRegister(read(address));
    SLA(reg);
    write(address, reg());
}

void Cpu::SRA(ByteRegister& reg) noexcept
{
    std::uint8_t const value  = reg();
    std::uint8_t const carry  = (value & 0x01);
    std::uint8_t const b7     = (value & 0x80);
    std::uint8_t const result = ((value >> 1) | b7);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, false);
    F_.setFlag(Flag::CARRY, carry != 0);
}

void Cpu::SRA(Address address) noexcept
{
    auto reg = ByteRegister(read(address));
    SRA(reg);
    write(address, reg());
}

void Cpu::SWAP(ByteRegister& reg) noexcept
{
    std::uint8_t const value  = reg();
    std::uint8_t const result = (((value & 0xF0) >> 4) | ((value & 0x0F) << 4));

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, false);
    F_.setFlag(Flag::CARRY, false);
}

void Cpu::SWAP(Address address) noexcept
{
    auto reg = ByteRegister(read(address));
    SWAP(reg);
    write(address, reg());
}

void Cpu::SRL(ByteRegister& reg) noexcept
{
    std::uint8_t const value  = reg();
    std::uint8_t const carry  = (value & 0x01);
    std::uint8_t const result = (value >> 1);

    reg = result;
    F_.setFlag(Flag::ZERO, result == 0);
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, false);
    F_.setFlag(Flag::CARRY, carry != 0);
}

void Cpu::SRL(Address address) noexcept
{
    auto reg = ByteRegister(read(address));
    SRL(reg);
    write(address, reg());
}

void Cpu::BIT(int bit, ByteRegister const& reg) noexcept
{
    assert((bit >= 0) && (bit <= 7));
    std::uint8_t const value = reg();

    F_.setFlag(Flag::ZERO, ((value & (1u << bit)) == 0));
    F_.setFlag(Flag::NEGATIVE, false);
    F_.setFlag(Flag::HALF_CARRY, true);
}

void Cpu::BIT(int bit, Address address) noexcept
{
    auto const reg = ByteRegister(read(address));
    BIT(bit, reg);
}

void Cpu::RES(int bit, ByteRegister& reg) const noexcept
{
    assert((bit >= 0) && (bit <= 7));
    std::uint8_t const value  = reg();
    std::uint8_t const result = (value & ~(1u << bit));

    reg = result;
}

void Cpu::RES(int bit, Address address) noexcept
{
    auto reg = ByteRegister(read(address));
    RES(bit, reg);
    write(address, reg());
}

void Cpu::SET(int bit, ByteRegister& reg) const noexcept
{
    assert((bit >= 0) && (bit <= 7));
    std::uint8_t const value  = reg();
    std::uint8_t const result = (value | (1u << bit));

    reg = result;
}

void Cpu::SET(int bit, Address address) noexcept
{
    auto reg = ByteRegister(read(address));
    SET(bit, reg);
    write(address, reg());
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