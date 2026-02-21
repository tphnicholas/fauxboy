#ifndef FAUXBOY_CPU_HPP
#define FAUXBOY_CPU_HPP

#include <cstdint>
#include <functional>

#include "address.hpp"
#include "register.hpp"
#include "util.hpp"

namespace fxb
{
class Bus;

struct CpuState
{
    std::uint8_t A   = 0;
    std::uint8_t B   = 0;
    std::uint8_t C   = 0;
    std::uint8_t D   = 0;
    std::uint8_t E   = 0;
    std::uint8_t F   = 0;
    std::uint8_t H   = 0;
    std::uint8_t L   = 0;
    std::uint16_t SP = 0;
    std::uint16_t PC = 0;
};

class OpcodeNotImplementedException : public NotImplementedException
{
private:
    std::uint16_t extendedOpcode_;

public:
    explicit OpcodeNotImplementedException(std::uint16_t extendedOpcode);

    [[nodiscard]] std::uint16_t opcode() const noexcept { return extendedOpcode_; }
};

class Cpu
{
public:
    using OnTickCallback = std::function<void(Cpu*)>;

private:
    enum class Flag : std::uint8_t
    {
        CARRY      = (1u << 4),
        HALF_CARRY = (1u << 5),
        NEGATIVE   = (1u << 6),
        ZERO       = (1u << 7)
    };

    Bus* bus_;

    ByteRegister A_;
    ByteRegister B_;
    ByteRegister C_;
    ByteRegister D_;
    ByteRegister E_;
    FlagRegister<Flag> F_;
    ByteRegister H_;
    ByteRegister L_;
    ShortRegister SP_;
    ShortRegister PC_;

    RegisterPairView AF_ = {&A_, &F_};
    RegisterPairView BC_ = {&B_, &C_};
    RegisterPairView DE_ = {&D_, &E_};
    RegisterPairView HL_ = {&H_, &L_};

    OnTickCallback onTick = nullptr;

private:
    [[nodiscard]] std::uint8_t read(Address address);
    void write(Address address, std::uint8_t value);

    [[nodiscard]] std::uint8_t readNextByteAndAdvance() noexcept;
    void execute(std::uint8_t opcode);
    void executeExtended(std::uint16_t opcode);

    void tick();

public:
    explicit Cpu(Bus* bus) noexcept;

    [[nodiscard]] std::uint8_t A() const noexcept { return A_(); }
    [[nodiscard]] std::uint8_t B() const noexcept { return B_(); }
    [[nodiscard]] std::uint8_t C() const noexcept { return C_(); }
    [[nodiscard]] std::uint8_t D() const noexcept { return D_(); }
    [[nodiscard]] std::uint8_t E() const noexcept { return E_(); }
    [[nodiscard]] std::uint8_t F() const noexcept { return F_(); }
    [[nodiscard]] std::uint8_t H() const noexcept { return H_(); }
    [[nodiscard]] std::uint8_t L() const noexcept { return L_(); }
    [[nodiscard]] std::uint16_t SP() const noexcept { return SP_(); }
    [[nodiscard]] std::uint16_t PC() const noexcept { return PC_(); }

    [[nodiscard]] std::uint16_t AF() const noexcept { return AF_(); }
    [[nodiscard]] std::uint16_t BC() const noexcept { return BC_(); }
    [[nodiscard]] std::uint16_t DE() const noexcept { return DE_(); }
    [[nodiscard]] std::uint16_t HL() const noexcept { return HL_(); }

    void setOnTickCallback(OnTickCallback callback);

    void reset(CpuState const& state = {});

    void step();
};
} // namespace fxb

#endif // FAUXBOY_CPU_HPP