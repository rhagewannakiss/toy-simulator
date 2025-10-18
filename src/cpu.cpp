#include "cpu.hpp"
#include "simulator.hpp"
#include "instructions.hpp"

#include <iostream>
#include <cstring>
#include <fstream>

namespace Sim {

    void CPU::reset() {
        memory_.clear();
        std::memset(regs_, 0, sizeof(regs_));
        pc_ = 0;
        halted_ = false;
        std::cerr << "CPU reset complete successfully.\n\n";
    }

    bool CPU::load_program(const std::string &path, Address base) {
        std::ifstream instr_file(path, std::ios::binary);
        if (!instr_file) {
            std::cerr << "CPU: Error in load program - cannot open file: " << path << "\n";
            return false;
        }

        instr_file.seekg(0, std::ios::end);
        std::streamsize file_size = instr_file.tellg();
        instr_file.seekg(0, std::ios::beg);
        if (file_size <= 0) {
            std::cerr << "CPU: Error in load programm - wrong file size (" << file_size << ").\n";
            return false;
        }

        memory_.resize(base + static_cast<size_t>(file_size));
        instr_file.read(reinterpret_cast<char*>(&memory_[base]), file_size);
        return true;
    }

    Instruction CPU::read(Address addr) {
        if (addr > memory_.size() - 4) {
            std::cerr << "Error in read: out-of-range read at 0x" << std::hex << addr << std::dec << "\n";
            return 0;
        }

        Opcode  b0 = memory_[addr];
        Opcode  b1 = memory_[addr + 1];
        Opcode  b2 = memory_[addr + 2];
        Opcode  b3 = memory_[addr + 3];

        return static_cast<Instruction>((b0) | (b1 << 8) | (b2 << 16) | (b3 << 24));
    }

    void CPU::write(Address addr, uint32_t value) {
        size_t need = static_cast<size_t>(addr) + 4;
        if (need > memory_.size()) {
            memory_.resize(need, 0);
        }

        memory_[addr]     = static_cast<Opcode>(value & 0xFFu);
        memory_[addr + 1] = static_cast<Opcode>((value >> 8) & 0xFFu);
        memory_[addr + 2] = static_cast<Opcode>((value >> 16) & 0xFFu);
        memory_[addr + 3] = static_cast<Opcode>((value >> 24) & 0xFFu);
    }

    Opcode CPU::opcode_of(Instruction instr) {
        return (static_cast<Opcode>((instr >> 26) & 0x3Fu));
    }

    Opcode CPU::func_of(Instruction instr) {
        return (static_cast<Opcode>(instr & 0x3Fu));
    }

    Register  CPU::sign_extend(Register  v) {
        if (v & 0x8000u) {
            return static_cast<Register >(v | 0xFFFF0000u);
        }

        return static_cast<Register >(v & 0x0000FFFFu);
    }

    Register  CPU::rot_r(Register  v, unsigned n) {
        n &= 31u;

        if (n == 0) {
            return v;
        }
        return static_cast<Register>((v >> n) | (v << (32 - n)));
    }

    Register CPU::pdep_emulate(Register src, uint32_t mask) {
        Register result = 0;

        for (unsigned mask_bit_pos = 0; mask_bit_pos < 32; ++mask_bit_pos) {
            uint32_t mask_bit = (mask >> mask_bit_pos) & 1u;
            if (mask_bit != 0u) {
                Register src_lsb = src & 1u;

                if (src_lsb != 0u) {
                    result |= (static_cast<Register>(1u) << mask_bit_pos);
                }
                src >>= 1;
            }
        }
        return result;
    }

    Register CPU::cls_emulate(Register x) {
        Register sign = (x >> 31) & 1u;

        unsigned count = 0;

        for (int pos = 31; pos >= 0; --pos) {
            Register bit = (x >> pos) & 1u;
            if (bit == sign) {
                ++count;
                if (count >= 31) {
                    return static_cast<Register>(31u);
                }
            } else {
                break;
            }
        }

        return static_cast<Register>(count);
    }

    void CPU::run() {
        while (!halted_) {
            step();
        }
    }

    void CPU::step() {
        Instruction instr = read(pc_);
        Address next_pc = pc_ + 4;

        Opcode opc = opcode_of(instr);

        switch (static_cast<InstrOpcodes>(opc)) {
            case InstrOpcodes::j:
                exec_j(instr, next_pc);
                break;
            case InstrOpcodes::stp:
                exec_stp(instr, next_pc);
                break;
            case InstrOpcodes::rori:
                exec_rori(instr, next_pc);
                break;
            case InstrOpcodes::slti:
                exec_slti(instr, next_pc);
                break;
            case InstrOpcodes::st:
                exec_st(instr, next_pc);
                break;
            case InstrOpcodes::bne:
                exec_bne(instr, next_pc);
                break;
            case InstrOpcodes::beq:
                exec_beq(instr, next_pc);
                break;
            case InstrOpcodes::ld:
                exec_ld(instr, next_pc);
                break;
            case InstrOpcodes::ssat:
                exec_ssat(instr, next_pc);
                break;
            case InstrOpcodes::syscall: {
                Opcode funct = func_of(instr);

                if (funct == static_cast<Opcode>(SubEncoding::add)) {
                    exec_add(instr, next_pc);
                }
                else if (funct == static_cast<Opcode>(SubEncoding::and_)) {
                    exec_and(instr, next_pc);
                }
                else if (funct == static_cast<Opcode>(SubEncoding::bdep)) {
                    exec_bdep(instr, next_pc);
                }
                else if (funct == static_cast<Opcode>(SubEncoding::cls)) {
                    exec_cls(instr, next_pc);
                }
                else if (funct == static_cast<Opcode>(SubEncoding::syscall)) {
                    exec_syscall(instr, next_pc);
                }
                else {
                    std::cerr << "Unknown funct: 0x" << std::hex << static_cast<int>(funct)
                            << std::dec << " at pc 0x" << std::hex << pc_ << std::dec
                            << ", CPU halted. \n";
                    halted_ = true;
                }
                break;
            }
            default:
                std::cerr << "Unknown opcode: 0x" << std::hex << int(opc) << std::dec << " at pc 0x" << std::hex << pc_ << std::dec << ", CPU halted. \n";
                halted_ = true;
                break;
        }

        pc_ = next_pc;
    }


    //---------------------- dump -------------------------
    void CPU::dump_regs() {
        std::ios::fmtflags f = std::cout.flags();
        std::cout << "----- CPU REGISTER DUMP -----\n";
        std::cout << "PC = 0x" << std::hex << pc_ << std::dec << "\n";

        for (Register i = 0; i < static_cast<Register>(kNumberOfRegisters); ++i) {
            std::cout << "X" << i << " = 0x" << std::hex << regs_[i] << std::dec;
            if ((i % 4) == 3) std::cout << "\n"; else std::cout << "\t";
        }
        std::cout << "\n----- END OF DUMP -----\n";
        std::cout.flags(f);
    }

    //------------------ instructions ---------------------
    void CPU::exec_j(Instruction instr, Address &next_pc) {
        Instruction instr_index = instr & 0x03FFFFFFu;
        next_pc = static_cast<Address>((pc_ & 0xF0000000u) | (instr_index << 2));
    }

    void CPU::exec_syscall(Instruction instr, Address &next_pc) {
        Register code = static_cast<Register>(instr >> 6) & 0x3FFFFu;

        if (code == 0) {
            halted_ = true;
        } else if (code == 1) {
            std::cout << regs_[0] << "\n";
        } else {
            std::cerr << "syscall: unhandled code " << code << "\n";
        }
    }

    void CPU::exec_stp(Instruction instr, Address &next_pc) {
        Register base = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rt1  = static_cast<Register>((instr >> 16) & 0x1Fu);
        Register rt2  = static_cast<Register>((instr >> 11) & 0x1Fu);
        Address offset = static_cast<Address>(instr & 0x7FFu);

         if (base >= kNumberOfRegisters
          || rt1 >= kNumberOfRegisters
          || rt2 >= kNumberOfRegisters) {
            std::cerr << "exec_stp: bad register index, halting.\n";
            halted_ = true;
            return;
        }

        if ((offset & 0x3u) != 0u) {
            std::cerr << "exec_stp:  lowest 2 bits of offset must be zero: 0x" << std::hex << offset << std::dec << ", halting.\n";
            halted_ = true;
            return;
        }

        Address addr = regs_[base] + offset;

        write(addr, regs_[rt1]);
        write(addr + 4, regs_[rt2]);
    }

    void CPU::exec_rori(Instruction instr, Address &next_pc) {
        Register rd = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rs = static_cast<Register>((instr >> 16) & 0x1Fu);
        unsigned imm5 = (instr >> 11) & 0x1Fu;

        if (rd >= kNumberOfRegisters
         || rs >= kNumberOfRegisters) {
            std::cerr << "exec_rori: bad register index, halting.\n";
            halted_ = true;
            return;
        }

        regs_[rd] = rot_r(regs_[rs], imm5);
    }

    void CPU::exec_slti(Instruction instr, Address &next_pc) {
        Register rs = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rt = static_cast<Register>((instr >> 16) & 0x1Fu);

        if (rs >= kNumberOfRegisters
         || rt >= kNumberOfRegisters) {
            std::cerr << "exec_slti: bad register index, halting.\n";
            halted_ = true;
            return;
        }

        int32_t imm = sign_extend(instr & 0xFFFFu);

        regs_[rt] = (static_cast<int32_t>(regs_[rs]) < imm) ? 1u : 0u;
    }

    void CPU::exec_st(Instruction instr, Address &next_pc) {
        Register base = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rt = static_cast<Register>((instr >> 16) & 0x1Fu);
        Address offset = static_cast<Address>(instr & 0xFFFFu);

        if (base >= kNumberOfRegisters
         || rt >= kNumberOfRegisters) {
            std::cerr << "exec_st: bad register index, halting.\n";
            halted_ = true;
            return;
        }

        if ((offset & 0x3u) != 0u) {
            std::cerr << "exec_st: lowest 2 bits of offset must be zero: 0x" << std::hex << offset << std::dec << ", halting.\n";
            halted_ = true;
            return;
        }

        Address addr = regs_[base] + offset;

        write(addr, regs_[rt]);
    }

    void CPU::exec_bdep(Instruction instr, Address &next_pc) {
        Register rd = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rs1 = static_cast<Register>((instr >> 16) & 0x1Fu);
        Register rs2 = static_cast<Register>((instr >> 11) & 0x1Fu);

        if (rd >= kNumberOfRegisters
            || rs1 >= kNumberOfRegisters
            || rs2 >= kNumberOfRegisters) {
            std::cerr << "exec_bdep: bad register index, halting.\n";
            halted_ = true;
            return;
        }

        regs_[rd] = pdep_emulate(regs_[rs1], regs_[rs2]);
    }

    void CPU::exec_cls(Instruction instr, Address &next_pc) {
        Register rd = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rs = static_cast<Register>((instr >> 16) & 0x1Fu);

        if (rd >= kNumberOfRegisters
            || rs >= kNumberOfRegisters) {
            std::cerr << "exec_cls: bad register index, halting.\n";
            halted_ = true;
            return;
        }

        regs_[rd] = cls_emulate(regs_[rs]);
    }

    void CPU::exec_add(Instruction instr, Address &next_pc) {
        Register rs = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rt = static_cast<Register>((instr >> 16) & 0x1Fu);
        Register rd = static_cast<Register>((instr >> 11) & 0x1Fu);

        if (rs >= kNumberOfRegisters
            || rt >= kNumberOfRegisters
            || rd >= kNumberOfRegisters) {
            std::cerr << "exec_add: bad register index, halting.\n";
            halted_ = true;
            return;
        }

        regs_[rd] = regs_[rs] + regs_[rt];
    }

    void CPU::exec_bne(Instruction instr, Address &next_pc) {
        Register rs = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rt = static_cast<Register>((instr >> 16) & 0x1Fu);

        if (rs >= kNumberOfRegisters
            || rt >= kNumberOfRegisters) {
            std::cerr << "exec_bne: bad register index, halting.\n";
            halted_ = true;
            return;
        }

        Instruction imm16 = instr & 0xFFFFu;
        Address offset = sign_extend(imm16) << 2;

        if (regs_[rs] != regs_[rt]) {
            next_pc = pc_ + offset;
        }
    }

    void CPU::exec_beq(Instruction instr, Address &next_pc) {
        Register rs = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rt = static_cast<Register>((instr >> 16) & 0x1Fu);

        if (rs >= kNumberOfRegisters
            || rt >= kNumberOfRegisters) {
            std::cerr << "exec_beq: bad register index, halting.\n";
            halted_ = true;
            return;
        }

        Instruction imm16 = instr & 0xFFFFu;
        Address offset = static_cast<Address>(sign_extend(imm16)) << 2;

        if (regs_[rs] == regs_[rt]) {
            next_pc = pc_ + offset;
        }
    }

    void CPU::exec_ld(Instruction instr, Address &next_pc) {
        Register base = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rt = static_cast<Register>((instr >> 16) & 0x1Fu);
        Address offset = static_cast<Address>(instr & 0xFFFFu);

        if (base >= kNumberOfRegisters
            || rt >= kNumberOfRegisters) {
            std::cerr << "exec_ld: bad register index, halting.\n";
            halted_ = true;
            return;
        }

        if ((offset & 0x3u) != 0u) {
            std::cerr << "exec_ld: lowest 2 bits of offset must be zero: 0x" << std::hex << offset << std::dec << ", halting.\n";
            halted_ = true;
            return;
        }

        Address addr = regs_[base] + offset;
        regs_[rt] = read(addr);
    }

    void CPU::exec_and(Instruction instr, Address &next_pc) {
        Register rs = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rt = static_cast<Register>((instr >> 16) & 0x1Fu);
        Register rd = static_cast<Register>((instr >> 11) & 0x1Fu);

        if (rs >= kNumberOfRegisters
            || rt >= kNumberOfRegisters
            || rd >= kNumberOfRegisters) {
            std::cerr << "exec_and: bad register index, halting.\n";
            halted_ = true;
            return;
        }

        regs_[rd] = regs_[rs] & regs_[rt];
    }

    void CPU::exec_ssat(Instruction instr, Address &next_pc) {
        Register rd = static_cast<Register>((instr >> 21) & 0x1Fu);
        Register rs = static_cast<Register>((instr >> 16) & 0x1Fu);
        Register imm5 = static_cast<Register>((instr >> 11) & 0x1Fu);

        if (rd >= kNumberOfRegisters
            || rs >= kNumberOfRegisters) {
            std::cerr << "exec_ssat: bad register index\n";
            halted_ = true;
            return;
        }

        Register N = imm5;
        if (N == 0) {
            regs_[rd] = regs_[rs];
            return;
        }

        int64_t minv = - (1LL << (N - 1));
        int64_t maxv =  (1LL << (N - 1)) - 1;

        int64_t val = static_cast<Register>(regs_[rs]);

        if (val < minv) val = minv;
        if (val > maxv) val = maxv;

        regs_[rd] = static_cast<Register>(val);
    }

} //namespace Sim