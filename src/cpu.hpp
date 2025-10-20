#ifndef CPU_HPP_
#define CPU_HPP_

#include "config.hpp"
#include "instructions.hpp"

#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>

namespace Sim {

class CPU {
private:
    std::vector<uint8_t> memory_;
    Register regs_[kNumberOfRegisters];
    Address pc_;
    bool halted_;

    Instruction read(Address pc_);
    void write(Address addr, uint32_t value);
    Opcode opcode_of(Instruction instr);
    Opcode func_of(Instruction instr);
    DecodedInstr decode_opcode(Instruction instr);

    Register sign_extend(Register v);
    Register rot_r(Register  v, unsigned n);
    Register pdep_emulate(Register  src, uint32_t mask);
    Register cls_emulate(Register  x);

    void exec_j(Instruction instr, Address &next_pc);
    void exec_syscall(Instruction instr, Address &next_pc);
    void exec_stp(Instruction instr, Address &next_pc);
    void exec_rori(Instruction instr, Address &next_pc);
    void exec_slti(Instruction instr, Address &next_pc);
    void exec_st(Instruction instr, Address &next_pc);
    void exec_bdep(Instruction instr, Address &next_pc);
    void exec_cls(Instruction instr, Address &next_pc);
    void exec_add(Instruction instr, Address &next_pc);
    void exec_bne(Instruction instr, Address &next_pc);
    void exec_beq(Instruction instr, Address &next_pc);
    void exec_ld(Instruction instr, Address &next_pc);
    void exec_and(Instruction instr, Address &next_pc);
    void exec_ssat(Instruction instr, Address &next_pc);

public:
    CPU() {};
    ~CPU() = default;

    void reset();
    bool load_program(const std::string &path, Address base = 0);
    void run();
    void step();
    void dump_regs();

    void set_PC(Address addr) {
        pc_ = addr;
    }

    void set_register(unsigned int idx, uint32_t value) {
        if (idx < kNumberOfRegisters) regs_[idx] = value;
    }

    Address get_PC() const {
        return pc_;
    }

    uint32_t get_register(unsigned int idx) const {
        if (idx < kNumberOfRegisters) return regs_[idx];
        return 0;
    }
};

} // namespace Sim

#endif //CPU_HPP_