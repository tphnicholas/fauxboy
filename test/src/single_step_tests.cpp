#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>

#include <simdjson.h>

#include <fauxboy/bus.hpp>
#include <fauxboy/cpu.hpp>
#include <fauxboy/address.hpp>
#include <fauxboy/util.hpp>

#include "config.hpp"

using namespace fxb;

namespace Catch
{
template <>
struct StringMaker<Address>
{
    static std::string convert(Address const& address) { return std::format("Address({})", address.value); }
};
} // namespace Catch

namespace
{
struct MemoryAccess
{
    Address address;
    std::uint8_t data;
    MemoryAccessMode accessMode;
};

class OpenBus : public Bus
{
private:
    std::vector<std::uint8_t> memory_;
    MemoryAccess lastMemoryAccess_ = {.address = Address(0), .data = 0, .accessMode = MemoryAccessMode::READ};

public:
    OpenBus()
        : memory_(1024 * 64, 0)
    {
    }

    [[nodiscard]] MemoryAccess const& getLastMemoryAccess() const noexcept { return lastMemoryAccess_; }

    void reset() { std::ranges::fill(memory_, 0); }

    [[nodiscard]] std::uint8_t read(Address address) override
    {
        lastMemoryAccess_.address    = address;
        lastMemoryAccess_.accessMode = MemoryAccessMode::READ;
        return memory_[address.value];
    }
    void write(Address address, std::uint8_t value) override
    {
        lastMemoryAccess_.address    = address;
        lastMemoryAccess_.data       = value;
        lastMemoryAccess_.accessMode = MemoryAccessMode::WRITE;
        memory_[address.value]       = value;
    }
};

struct RamSlot
{
    Address address;
    std::uint8_t value;
};

struct SystemState
{
    CpuState cpuState;
    std::vector<RamSlot> ram;
};

struct BusState
{
    Address address;
    std::uint8_t data;
    std::string_view accessMode;
};

struct TestData
{
    std::string_view name;
    SystemState initial;
    SystemState final;
    std::vector<BusState> cycles;
};

class SingleStepTestsFixture
{
private:
    mutable simdjson::ondemand::parser parser_;
    mutable std::string buffer_;
    mutable std::filesystem::path filepath_ = (Config::SINGLE_STEP_TESTS_DIR / "dummy.json");

public:
    // Illegal opcodes and PREFIX instructions are ignored, nothing to test
    static inline std::vector<std::uint16_t> opcodes = {
        // clang-format off
        // Unprefixed
        0x00, 0x01, 0x02,   0x03,   0x04,   0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,   0x0B,   0x0C,   0x0D,   0x0E, 0x0F,
        0x10, 0x11, 0x12,   0x13,   0x14,   0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,   0x1B,   0x1C,   0x1D,   0x1E, 0x1F,
        0x20, 0x21, 0x22,   0x23,   0x24,   0x25, 0x26, 0x27, 0x28, 0x29, 0x2A,   0x2B,   0x2C,   0x2D,   0x2E, 0x2F,
        0x30, 0x31, 0x32,   0x33,   0x34,   0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,   0x3B,   0x3C,   0x3D,   0x3E, 0x3F,
        0x40, 0x41, 0x42,   0x43,   0x44,   0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,   0x4B,   0x4C,   0x4D,   0x4E, 0x4F,
        0x50, 0x51, 0x52,   0x53,   0x54,   0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,   0x5B,   0x5C,   0x5D,   0x5E, 0x5F,
        0x60, 0x61, 0x62,   0x63,   0x64,   0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,   0x6B,   0x6C,   0x6D,   0x6E, 0x6F,
        0x70, 0x71, 0x72,   0x73,   0x74,   0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,   0x7B,   0x7C,   0x7D,   0x7E, 0x7F,
        0x80, 0x81, 0x82,   0x83,   0x84,   0x85, 0x86, 0x87, 0x88, 0x89, 0x8A,   0x8B,   0x8C,   0x8D,   0x8E, 0x8F,
        0x90, 0x91, 0x92,   0x93,   0x94,   0x95, 0x96, 0x97, 0x98, 0x99, 0x9A,   0x9B,   0x9C,   0x9D,   0x9E, 0x9F,
        0xA0, 0xA1, 0xA2,   0xA3,   0xA4,   0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA,   0xAB,   0xAC,   0xAD,   0xAE, 0xAF,
        0xB0, 0xB1, 0xB2,   0xB3,   0xB4,   0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA,   0xBB,   0xBC,   0xBD,   0xBE, 0xBF,
        0xC0, 0xC1, 0xC2,   0xC3,   0xC4,   0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, /*0xCB,*/ 0xCC,   0xCD,   0xCE, 0xCF,
        0xD0, 0xD1, 0xD2, /*0xD3,*/ 0xD4,   0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, /*0xDB,*/ 0xDC, /*0xDD,*/ 0xDE, 0xDF,
        0xE0, 0xE1, 0xE2, /*0xE3,   0xE4,*/ 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, /*0xEB,   0xEC,   0xED,*/ 0xEE, 0xEF,
        0xF0, 0xF1, 0xF2,   0xF3, /*0xF4,*/ 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,   0xFB, /*0xFC,   0xFD,*/ 0xFE, 0xFF,
        // 0xCB Prefixed
        0xCB00, 0xCB01, 0xCB02, 0xCB03, 0xCB04, 0xCB05, 0xCB06, 0xCB07, 0xCB08, 0xCB09, 0xCB0A, 0xCB0B, 0xCB0C, 0xCB0D, 0xCB0E, 0xCB0F,
        0xCB10, 0xCB11, 0xCB12, 0xCB13, 0xCB14, 0xCB15, 0xCB16, 0xCB17, 0xCB18, 0xCB19, 0xCB1A, 0xCB1B, 0xCB1C, 0xCB1D, 0xCB1E, 0xCB1F,
        0xCB20, 0xCB21, 0xCB22, 0xCB23, 0xCB24, 0xCB25, 0xCB26, 0xCB27, 0xCB28, 0xCB29, 0xCB2A, 0xCB2B, 0xCB2C, 0xCB2D, 0xCB2E, 0xCB2F,
        0xCB30, 0xCB31, 0xCB32, 0xCB33, 0xCB34, 0xCB35, 0xCB36, 0xCB37, 0xCB38, 0xCB39, 0xCB3A, 0xCB3B, 0xCB3C, 0xCB3D, 0xCB3E, 0xCB3F,
        0xCB40, 0xCB41, 0xCB42, 0xCB43, 0xCB44, 0xCB45, 0xCB46, 0xCB47, 0xCB48, 0xCB49, 0xCB4A, 0xCB4B, 0xCB4C, 0xCB4D, 0xCB4E, 0xCB4F,
        0xCB50, 0xCB51, 0xCB52, 0xCB53, 0xCB54, 0xCB55, 0xCB56, 0xCB57, 0xCB58, 0xCB59, 0xCB5A, 0xCB5B, 0xCB5C, 0xCB5D, 0xCB5E, 0xCB5F,
        0xCB60, 0xCB61, 0xCB62, 0xCB63, 0xCB64, 0xCB65, 0xCB66, 0xCB67, 0xCB68, 0xCB69, 0xCB6A, 0xCB6B, 0xCB6C, 0xCB6D, 0xCB6E, 0xCB6F,
        0xCB70, 0xCB71, 0xCB72, 0xCB73, 0xCB74, 0xCB75, 0xCB76, 0xCB77, 0xCB78, 0xCB79, 0xCB7A, 0xCB7B, 0xCB7C, 0xCB7D, 0xCB7E, 0xCB7F,
        0xCB80, 0xCB81, 0xCB82, 0xCB83, 0xCB84, 0xCB85, 0xCB86, 0xCB87, 0xCB88, 0xCB89, 0xCB8A, 0xCB8B, 0xCB8C, 0xCB8D, 0xCB8E, 0xCB8F,
        0xCB90, 0xCB91, 0xCB92, 0xCB93, 0xCB94, 0xCB95, 0xCB96, 0xCB97, 0xCB98, 0xCB99, 0xCB9A, 0xCB9B, 0xCB9C, 0xCB9D, 0xCB9E, 0xCB9F,
        0xCBA0, 0xCBA1, 0xCBA2, 0xCBA3, 0xCBA4, 0xCBA5, 0xCBA6, 0xCBA7, 0xCBA8, 0xCBA9, 0xCBAA, 0xCBAB, 0xCBAC, 0xCBAD, 0xCBAE, 0xCBAF,
        0xCBB0, 0xCBB1, 0xCBB2, 0xCBB3, 0xCBB4, 0xCBB5, 0xCBB6, 0xCBB7, 0xCBB8, 0xCBB9, 0xCBBA, 0xCBBB, 0xCBBC, 0xCBBD, 0xCBBE, 0xCBBF,
        0xCBC0, 0xCBC1, 0xCBC2, 0xCBC3, 0xCBC4, 0xCBC5, 0xCBC6, 0xCBC7, 0xCBC8, 0xCBC9, 0xCBCA, 0xCBCB, 0xCBCC, 0xCBCD, 0xCBCE, 0xCBCF,
        0xCBD0, 0xCBD1, 0xCBD2, 0xCBD3, 0xCBD4, 0xCBD5, 0xCBD6, 0xCBD7, 0xCBD8, 0xCBD9, 0xCBDA, 0xCBDB, 0xCBDC, 0xCBDD, 0xCBDE, 0xCBDF,
        0xCBE0, 0xCBE1, 0xCBE2, 0xCBE3, 0xCBE4, 0xCBE5, 0xCBE6, 0xCBE7, 0xCBE8, 0xCBE9, 0xCBEA, 0xCBEB, 0xCBEC, 0xCBED, 0xCBEE, 0xCBEF,
        0xCBF0, 0xCBF1, 0xCBF2, 0xCBF3, 0xCBF4, 0xCBF5, 0xCBF6, 0xCBF7, 0xCBF8, 0xCBF9, 0xCBFA, 0xCBFB, 0xCBFC, 0xCBFD, 0xCBFE, 0xCBFF,
        // clang-format on
    };

    mutable OpenBus bus;
    mutable Cpu cpu = Cpu(&bus);

    mutable simdjson::ondemand::document document;
    mutable TestData testData;

private:
    void setFilepathFromOpcode(std::uint16_t opcode) const
    {
        std::uint8_t const prefix = getUpper(opcode);
        std::uint8_t const offset = getLower(opcode);

        switch (prefix)
        {
            case 0x00:
            {
                filepath_.replace_filename(std::format("{:02x}.json", offset));
                break;
            }
            default:
            {
                filepath_.replace_filename(std::format("{:02x} {:02x}.json", prefix, offset));
                break;
            }
        }
    }

    void loadFile() const
    {
        auto ifs = std::ifstream(filepath_, std::ios::binary);
        if (!ifs.is_open())
        {
            throw std::runtime_error(std::format("Could not open file: {}", filepath_.c_str()));
        }

        ifs.seekg(0, std::ios::end);
        auto const fileSize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        buffer_.resize(fileSize, '\0');
        std::copy(std::istreambuf_iterator(ifs), {}, buffer_.begin());

        document = parser_.iterate(simdjson::pad(buffer_));
    }

    template <typename T, typename U, typename Itr>
    void parseNextAs(Itr& itr, U& out) const
    {
        T result = (*itr).template get<T>();
        out      = U(result);
        ++itr;
    }

    void parseSystemState(simdjson::ondemand::object json, SystemState& systemState) const
    {
        auto& cpuState = systemState.cpuState;
        cpuState.A     = json["a"].get<std::uint8_t>();
        cpuState.B     = json["b"].get<std::uint8_t>();
        cpuState.C     = json["c"].get<std::uint8_t>();
        cpuState.D     = json["d"].get<std::uint8_t>();
        cpuState.E     = json["e"].get<std::uint8_t>();
        cpuState.F     = json["f"].get<std::uint8_t>();
        cpuState.H     = json["h"].get<std::uint8_t>();
        cpuState.L     = json["l"].get<std::uint8_t>();
        cpuState.PC    = json["pc"].get<std::uint16_t>();
        cpuState.SP    = json["sp"].get<std::uint16_t>();

        auto& ram = systemState.ram;

        ram.clear();
        for (simdjson::ondemand::array slotJson : json["ram"])
        {
            auto itr   = slotJson.begin();
            auto& slot = ram.emplace_back();
            parseNextAs<std::uint16_t>(itr, slot.address);
            parseNextAs<std::uint8_t>(itr, slot.value);
        }
    }

    template <typename T>
    void parseCycles(simdjson::ondemand::array json, T& cycles) const
    {
        cycles.clear();
        for (simdjson::ondemand::array cycleJson : json)
        {
            auto itr    = cycleJson.begin();
            auto& cycle = cycles.emplace_back();
            parseNextAs<std::uint16_t>(itr, cycle.address);
            parseNextAs<std::uint8_t>(itr, cycle.data);
            parseNextAs<std::string_view>(itr, cycle.accessMode);
        }
    }

public:
    void resetFixtureForOpcode(std::uint16_t opcode) const
    {
        setFilepathFromOpcode(opcode);
        loadFile();
    }

    void loadTestData(simdjson::ondemand::object json) const
    {
        testData.name = json["name"];
        parseSystemState(json["initial"], testData.initial);
        parseSystemState(json["final"], testData.final);
        parseCycles(json["cycles"], testData.cycles);
    }
};
} // namespace

TEST_CASE_PERSISTENT_FIXTURE(SingleStepTestsFixture, "SingleStepTests", "[.][single-step-tests]")
{
    for (auto opcode : opcodes)
    {
        INFO("opcode: " << ((getUpper(opcode) != 0x00) ? std::format("0x{:04X}", opcode)
                                                       : std::format("0x{:02X}", getLower(opcode))));

        REQUIRE_NOTHROW(resetFixtureForOpcode(opcode));

        int cycleCount = 0;

        cpu.setOnTickCallback(
            [this, &cycleCount](Cpu const*)
            {
                auto const& lastMemoryAccess = bus.getLastMemoryAccess();
                auto const& cycle            = testData.cycles[cycleCount++];

                if (cycle.accessMode == "r-m")
                {
                    REQUIRE(lastMemoryAccess.accessMode == MemoryAccessMode::READ);
                    REQUIRE(lastMemoryAccess.address == cycle.address);
                }
                else if (cycle.accessMode == "-wm")
                {
                    REQUIRE(lastMemoryAccess.accessMode == MemoryAccessMode::WRITE);
                    REQUIRE(lastMemoryAccess.address == cycle.address);
                    REQUIRE(lastMemoryAccess.data == cycle.data);
                }
                else
                {
                    // Explicitly ignore checking the last memory access during internal cycles
                    REQUIRE(cycle.accessMode == "---");
                }
            });

        simdjson::ondemand::array json = document;
        for (simdjson::ondemand::object testJson : json)
        {
            REQUIRE_NOTHROW(loadTestData(testJson));

            auto const& initial = testData.initial;
            auto const& final   = testData.final;

            INFO("test: " << testData.name);

            bus.reset();
            for (auto const& slot : initial.ram)
            {
                bus.write(slot.address, slot.value);
            }

            cycleCount = 0;
            cpu.reset(initial.cpuState);

            REQUIRE_NOTHROW(cpu.step());

            REQUIRE(cycleCount == static_cast<int>(testData.cycles.size()));

            REQUIRE(cpu.A() == final.cpuState.A);
            REQUIRE(cpu.B() == final.cpuState.B);
            REQUIRE(cpu.C() == final.cpuState.C);
            REQUIRE(cpu.D() == final.cpuState.D);
            REQUIRE(cpu.E() == final.cpuState.E);
            REQUIRE(cpu.F() == final.cpuState.F);
            REQUIRE(cpu.H() == final.cpuState.H);
            REQUIRE(cpu.L() == final.cpuState.L);
            REQUIRE(cpu.SP() == final.cpuState.SP);
            REQUIRE(cpu.PC() == final.cpuState.PC);

            REQUIRE(cpu.AF() == ((final.cpuState.A << 8) | final.cpuState.F));
            REQUIRE(cpu.BC() == ((final.cpuState.B << 8) | final.cpuState.C));
            REQUIRE(cpu.DE() == ((final.cpuState.D << 8) | final.cpuState.E));
            REQUIRE(cpu.HL() == ((final.cpuState.H << 8) | final.cpuState.L));

            for (auto const& slot : final.ram)
            {
                REQUIRE(bus.read(slot.address) == slot.value);
            }
        }

        cpu.setOnTickCallback(nullptr);
    }
}