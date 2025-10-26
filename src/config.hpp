#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <cstdlib>
#include <cstdint>
#include <climits>

namespace Sim {

constexpr size_t kNumberOfRegisters = 32;
constexpr size_t kInstructionBytes = 4;
constexpr size_t kNumberOfBits = kInstructionBytes * CHAR_BIT;
constexpr size_t kBaseOfNumSys10 = 10;
constexpr size_t kBaseOfNumSys16 = 16;

using Register = uint32_t;
using Address = uint32_t;
using Instruction = uint32_t;
using Opcode = uint8_t;

}

#endif // CONFIG_HPP_