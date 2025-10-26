#ifndef CPU_HPP_
#define CPU_HPP_

#include "config.hpp"
#include "instructions.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

namespace Sim {

class CPU {
private:
    std::vector<Byte> memory_;
    Register regs_[kNumberOfRegisters];
    Address pc_;
    bool halted_;

    Instruction read(Address pc_);
    Opcode opcode_of(Instruction instr);
    Opcode func_of(Instruction instr);
    DecodedInstr decode_opcode(Instruction instr);

    Register_idx sign_extend(Register_idx v);
    Register_idx rot_r(Register_idx v, Register n);
    Register_idx pdep_emulate(Register_idx src, uint32_t mask);
    Register_idx cls_emulate(Register_idx x);

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
    bool load_program(const std::filesystem::path &path, Address base = 0);
    void write(Address addr, Register value);
    void run();
    void step();

    void set_PC(const Address addr) {
        pc_ = addr;
    }

    void set_register(const Register_idx idx, const Register value) {
        if (idx >= kNumberOfRegisters) {
            throw std::out_of_range("Register index out of range/n");
        }
        regs_[idx] = value;
    }

    Address get_PC() const {
        return pc_;
    }

    Register get_register(Register_idx idx) const {
        if (idx >= kNumberOfRegisters) {
             throw std::out_of_range("Register index out of range/n");
        }
        return regs_[idx];
    }

    void dump_regs() const;
};

} // namespace Sim

#endif //CPU_HPP_