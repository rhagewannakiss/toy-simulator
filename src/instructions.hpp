#ifndef INSTRUCTIONS_HPP_
#define INSTRUCTIONS_HPP_

namespace Sim {

    enum class InstrOpcodes {
        j =       0b100111,
        syscall = 0b000000, //*
        stp =     0b001000,
        rori =    0b111011,
        slti =    0b010110,
        st =      0b000011,
        bdep =    0b000000, //*
        cls =     0b000000, //*
        add =     0b000000, //*
        bne =     0b100100,
        beq =     0b000010,
        ld =      0b011010,
        and_ =    0b000000, //*
        ssat =    0b001111
    };

    enum class SubEncoding {
        syscall = 0b111110,
        bdep =    0b111010,
        cls =     0b111001,
        add =     0b101011,
        and_ =    0b101101
    };

} //namespace Sim

#endif //INSTRUCTIONS_HPP_