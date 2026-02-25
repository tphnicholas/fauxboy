#include "bus.hpp"

#include <stdexcept>
#include <format>
#include <utility>

#include "address.hpp"

namespace fxb
{
BadMemoryAccessException::BadMemoryAccessException(Address address, MemoryAccessMode accessMode)
    : std::runtime_error(std::format(
          "Bad memory access: on {} at 0x{:04X}",
          [accessMode]
          {
              using enum MemoryAccessMode;
              switch (accessMode)
              {
                  case READ: return "READ";
                  case WRITE: return "WRITE";
              }
              std::unreachable();
          }(), // IILE
          address.value)),
      address_(address),
      accessMode_(accessMode)
{
}
} // namespace fxb