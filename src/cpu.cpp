#include "cpu.hpp"
#include "instructions.hpp"

#include <iostream>
#include <cstring>
#include <fstream>
#include <filesystem>

namespace Sim {

void CPU::reset() {
    memory_.clear();
    std::memset(regs_, 0, sizeof(regs_));
    pc_ = 0;
    halted_ = false;
    std::cerr << "CPU reset complete successfully.\n\n";
}

bool CPU::load_program(const std::filesystem::path &path, Address base) {
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

void CPU::run() {
    while (!halted_) {
        step();
    }
}

void CPU::step() {
    Instruction instr = read(pc_);
    Address next_pc = pc_ + kInstructionBytes;

    DecodedInstr decoded = decode_opcode(instr);

    switch (decoded) {
        case DecodedInstr::j:
            exec_j(instr, next_pc);
            break;
        case DecodedInstr::syscall:
            exec_syscall(instr, next_pc);
            break;
        case DecodedInstr::stp:
            exec_stp(instr, next_pc);
            break;
        case DecodedInstr::rori:
            exec_rori(instr, next_pc);
            break;
        case DecodedInstr::slti:
            exec_slti(instr, next_pc);
            break;
        case DecodedInstr::st:
            exec_st(instr, next_pc);
            break;
        case DecodedInstr::bdep:
            exec_bdep(instr, next_pc);
            break;
        case DecodedInstr::cls:
            exec_cls(instr, next_pc);
            break;
        case DecodedInstr::add:
            exec_add(instr, next_pc);
            break;
        case DecodedInstr::bne:
            exec_bne(instr, next_pc);
            break;
        case DecodedInstr::beq:
            exec_beq(instr, next_pc);
            break;
        case DecodedInstr::ld:
            exec_ld(instr, next_pc);
            break;
        case DecodedInstr::and_:
            exec_and(instr, next_pc);
            break;
        case DecodedInstr::ssat:
            exec_ssat(instr, next_pc);
            break;
        case DecodedInstr::unknown:
            std::cerr << "Unknown decoded opcode at pc 0x" << std::hex << pc_ << std::dec << ", CPU halted.\n";
            halted_ = true;
            break;
    }

    pc_ = next_pc;
}

Instruction CPU::read(Address addr) {
    if (addr > memory_.size() - kInstructionBytes) {
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
    size_t need = static_cast<size_t>(addr) + kInstructionBytes;
    if (need > memory_.size()) {
        memory_.resize(need, 0);
    }

    memory_[addr]     = static_cast<Opcode>(value & 0x0000'00FF);
    memory_[addr + 1] = static_cast<Opcode>((value >> 8) & 0x0000'00FF);
    memory_[addr + 2] = static_cast<Opcode>((value >> 16) & 0x0000'00FF);
    memory_[addr + 3] = static_cast<Opcode>((value >> 24) & 0x0000'00FF);
}

Opcode CPU::opcode_of(Instruction instr) {
    return (static_cast<Opcode>((instr >> 26) & 0x0000'003F));
}

Opcode CPU::func_of(Instruction instr) {
    return (static_cast<Opcode>(instr & 0x0000'003F));
}

Register_idx CPU::sign_extend(Register_idx v) {
    if (v & 0x0000'8000) {
        return static_cast<Register_idx>(v | 0xFFFF'0000);
    }
    return static_cast<Register_idx>(v & 0x0000'FFFF);
}

Register_idx CPU::rot_r(Register_idx v, Register n) {
    n &= 0x0000'001F;

    if (n == 0) {
        return v;
    }
    return static_cast<Register_idx>((v >> n) | (v << (kNumberOfBits - n)));
}

Register_idx CPU::pdep_emulate(Register_idx src, uint32_t mask) {
    Register_idx result = 0;

    for (uint32_t mask_bit_pos = 0; mask_bit_pos < kNumberOfBits; ++mask_bit_pos) {
        uint32_t mask_bit = (mask >> mask_bit_pos) & 0x0000'0001;
        if (mask_bit != 0x0000'0000) {
            Register_idx src_lsb = src & 0x0000'0001;

            if (src_lsb != 0) {
                result |= (static_cast<Register_idx>(0x0000'0001) << mask_bit_pos);
            }
            src >>= 1;
        }
    }
    return result;
}

Register_idx CPU::cls_emulate(Register_idx x) {
    Register_idx sign = (x >> kNumberOfBits - 1) & 0x0000'0001;

    uint32_t count = 0;
    for (int pos = kNumberOfBits - 1; pos >= 0; --pos) {
        Register_idx bit = (x >> pos) & 0x0000'0001;
        if (bit == sign) {
            ++count;
            if (count >= kNumberOfBits - 1) {
                return static_cast<Register_idx>(0x0000'001F);
            }
        } else {
            break;
        }
    }

    return static_cast<Register_idx>(count);
}

DecodedInstr CPU::decode_opcode(Instruction instr) {
    Opcode op = opcode_of(instr);
    InstrOpcodes upper_bits = static_cast<InstrOpcodes>(op);
    if (upper_bits != InstrOpcodes::syscall) {
        switch (upper_bits) {
            case InstrOpcodes::j:
                return DecodedInstr::j;
            case InstrOpcodes::stp:
                return DecodedInstr::stp;
            case InstrOpcodes::rori:
                return DecodedInstr::rori;
            case InstrOpcodes::slti:
                return DecodedInstr::slti;
            case InstrOpcodes::st:
                return DecodedInstr::st;
            case InstrOpcodes::bne:
                return DecodedInstr::bne;
            case InstrOpcodes::beq:
                return DecodedInstr::beq;
            case InstrOpcodes::ld:
                return DecodedInstr::ld;
            case InstrOpcodes::ssat:
                return DecodedInstr::ssat;
            default:
                std::cerr << "Unknown primary opcode: 0x" << std::hex << int(op) << " at pc 0x" << pc_ << std::dec << "\n";
                halted_ = true;
                return DecodedInstr::unknown;
        }
    }

    Opcode func = func_of(instr);
    switch (static_cast<SubEncoding>(func)) {
        case SubEncoding::add:
            return DecodedInstr::add;
        case SubEncoding::and_:
            return DecodedInstr::and_;
        case SubEncoding::bdep:
            return DecodedInstr::bdep;
        case SubEncoding::cls:
            return DecodedInstr::cls;
        case SubEncoding::syscall:
            return DecodedInstr::syscall;
        default:
            std::cerr << "Unknown subencoding: 0x" << std::hex << int(func) << " at pc 0x" << pc_ << std::dec << "\n";
            halted_ = true;
            return DecodedInstr::unknown;
    }
}

//---------------------- dump -------------------------
void CPU::dump_regs() const {
    std::ios::fmtflags f = std::cout.flags();
    std::cout << "----- CPU REGISTER DUMP -----\n";
    std::cout << "PC = 0x" << std::hex << pc_ << std::dec << "\n";

    for (Register_idx i = 0; i < static_cast<Register_idx>(kNumberOfRegisters); ++i) {
        std::cout << "X" << i << " = 0x" << std::hex << regs_[i] << std::dec;
        if ((i % kInstructionBytes) == 3) std::cout << "\n"; else std::cout << "\t";
    }
    std::cout << "\n----- END OF DUMP -----\n";
    std::cout.flags(f);
}

//------------------ instructions ---------------------
void CPU::exec_j(Instruction instr, Address &next_pc) {
    Instruction instr_index = instr & 0x03FF'FFFF;
    next_pc = static_cast<Address>((pc_ & 0xF000'0000) | (instr_index << 2));
}

void CPU::exec_syscall(Instruction instr, Address &next_pc) {
    Register_idx code = static_cast<Register_idx>(instr >> 6) & 0x0003'FFFF;

    if (code == 0) {
        halted_ = true;
    } else if (code == 1) {
        std::cout << regs_[0] << "\n";
    } else {
        std::cerr << "syscall: unhandled code " << code << "\n";
    }
}

void CPU::exec_stp(Instruction instr, Address &next_pc) {
    Register_idx base = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rt1 = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);
    Register_idx rt2 = static_cast<Register_idx>((instr >> 11) & 0x0000'001F);
    Address offset = static_cast<Address>(instr & 0x0000'07FF);

    if (base >= kNumberOfRegisters
        || rt1 >= kNumberOfRegisters
        || rt2 >= kNumberOfRegisters) {
    std::cerr << "exec_stp: bad register index, halting.\n";
    halted_ = true;
    return;
    }
    Address addr = regs_[base] + offset;

    if ((addr & 0x0000'0003) != 0) {
        std::cerr << "exec_stp:  lowest 2 bits of offset must be zero: 0x" << std::hex << offset << std::dec << ", halting.\n";
        halted_ = true;
        return;
    }

    write(addr, regs_[rt1]);
    write(addr + kInstructionBytes, regs_[rt2]);
}

void CPU::exec_rori(Instruction instr, Address &next_pc) {
    Register_idx rd = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rs = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);
    uint32_t imm5 = (instr >> 11) & 0x0000'001F;

    if (rd >= kNumberOfRegisters
        || rs >= kNumberOfRegisters) {
        std::cerr << "exec_rori: bad register index, halting.\n";
        halted_ = true;
        return;
    }

    regs_[rd] = rot_r(regs_[rs], imm5);
}

void CPU::exec_slti(Instruction instr, Address &next_pc) {
    Register_idx rs = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rt = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);

    if (rs >= kNumberOfRegisters
        || rt >= kNumberOfRegisters) {
        std::cerr << "exec_slti: bad register index, halting.\n";
        halted_ = true;
        return;
    }

    int32_t imm = sign_extend(instr & 0x0000'FFFF);

    regs_[rt] = (static_cast<int32_t>(regs_[rs]) < imm) ? 0x0000'0001 : 0x0000'0000;
}

void CPU::exec_st(Instruction instr, Address &next_pc) {
    Register_idx base = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rt = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);
    Address offset = static_cast<Address>(instr & 0x0000'FFFF);

    if (base >= kNumberOfRegisters
        || rt >= kNumberOfRegisters) {
        std::cerr << "exec_st: bad register index, halting.\n";
        halted_ = true;
        return;
    }

    if ((offset & 0x3u) != 0x0000'0000) {
        std::cerr << "exec_st: lowest 2 bits of offset must be zero: 0x" << std::hex << offset << std::dec << ", halting.\n";
        halted_ = true;
        return;
    }

    Address addr = regs_[base] + offset;

    write(addr, regs_[rt]);
}

void CPU::exec_bdep(Instruction instr, Address &next_pc) {
    Register_idx rd = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rs1 = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);
    Register_idx rs2 = static_cast<Register_idx>((instr >> 11) & 0x0000'001F);

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
    Register_idx rd = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rs = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);

    if (rd >= kNumberOfRegisters
        || rs >= kNumberOfRegisters) {
        std::cerr << "exec_cls: bad register index, halting.\n";
        halted_ = true;
        return;
    }

    regs_[rd] = cls_emulate(regs_[rs]);
}

void CPU::exec_add(Instruction instr, Address &next_pc) {
    Register_idx rs = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rt = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);
    Register_idx rd = static_cast<Register_idx>((instr >> 11) & 0x0000'001F);

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
    Register_idx rs = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rt = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);

    if (rs >= kNumberOfRegisters
        || rt >= kNumberOfRegisters) {
        std::cerr << "exec_bne: bad register index, halting.\n";
        halted_ = true;
        return;
    }

    Instruction imm16 = instr & 0x0000'FFFF;
    Address offset = sign_extend(imm16) << 2;

    if (regs_[rs] != regs_[rt]) {
        next_pc = pc_ + offset;
    }
}

void CPU::exec_beq(Instruction instr, Address &next_pc) {
    Register_idx rs = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rt = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);

    if (rs >= kNumberOfRegisters
        || rt >= kNumberOfRegisters) {
        std::cerr << "exec_beq: bad register index, halting.\n";
        halted_ = true;
        return;
    }

    Instruction imm16 = instr & 0x0000'FFFF;
    Address offset = static_cast<Address>(sign_extend(imm16)) << 2;

    if (regs_[rs] == regs_[rt]) {
        next_pc = pc_ + offset;
    }
}

void CPU::exec_ld(Instruction instr, Address &next_pc) {
    Register_idx base = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rt = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);
    Address offset = static_cast<Address>(instr & 0x0000'FFFF);

    if (base >= kNumberOfRegisters
        || rt >= kNumberOfRegisters) {
        std::cerr << "exec_ld: bad register index, halting.\n";
        halted_ = true;
        return;
    }

    if ((offset & 0x0000'0003) != 0x0000'0000) {
        std::cerr << "exec_ld: lowest 2 bits of offset must be zero: 0x" << std::hex << offset << std::dec << ", halting.\n";
        halted_ = true;
        return;
    }

    Address addr = regs_[base] + offset;
    regs_[rt] = read(addr);
}

void CPU::exec_and(Instruction instr, Address &next_pc) {
    Register_idx rs = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rt = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);
    Register_idx rd = static_cast<Register_idx>((instr >> 11) & 0x0000'001F);

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
    Register_idx rd = static_cast<Register_idx>((instr >> 21) & 0x0000'001F);
    Register_idx rs = static_cast<Register_idx>((instr >> 16) & 0x0000'001F);
    Register_idx imm5 = static_cast<Register_idx>((instr >> 11) & 0x0000'001F);

    if (rd >= kNumberOfRegisters
        || rs >= kNumberOfRegisters) {
        std::cerr << "exec_ssat: bad register index\n";
        halted_ = true;
        return;
    }

    const Register_idx N = imm5 & 0x1Fu;
    if (N == 0) {
        regs_[rd] = regs_[rs];
        return;
    }

    const int64_t minv = -(1LL << (N - 1));
    const int64_t maxv = (1LL << (N - 1)) - 1;

    int64_t val = static_cast<int64_t>(static_cast<int32_t>(regs_[rs]));

    if (val < minv) val = minv;
    if (val > maxv) val = maxv;

    regs_[rd] = static_cast<Register_idx>(static_cast<int32_t>(val));
}

} // namespace Sim