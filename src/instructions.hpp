#ifndef INSTRUCTIONS_HPP_
#define INSTRUCTIONS_HPP_

namespace Sim {

enum class InstrOpcodes {
    j =       0b1001'11,
    syscall = 0b0000'00, //*
    stp =     0b0010'00,
    rori =    0b1110'11,
    slti =    0b0101'10,
    st =      0b0000'11,
    bdep =    0b0000'00, //*
    cls =     0b0000'00, //*
    add =     0b0000'00, //*
    bne =     0b1001'00,
    beq =     0b0000'10,
    ld =      0b0110'10,
    and_ =    0b0000'00, //*
    ssat =    0b0011'11
};

enum class SubEncoding {
    syscall = 0b1111'10,
    bdep =    0b1110'10,
    cls =     0b1110'01,
    add =     0b1010'11,
    and_ =    0b1011'01
};

enum class DecodedInstr {
    J,
    SYSCALL,
    STP,
    RORI,
    SLTI,
    ST,
    BDEP,
    CLS,
    ADD,
    BNE,
    BEQ,
    LD,
    AND,
    SSAT,
    UNKNOWN
};

} // namespace Sim

#endif //INSTRUCTIONS_HPP_