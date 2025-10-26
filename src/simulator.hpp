#ifndef SIMULATOR_HPP_
#define SIMULATOR_HPP_

#include "config.hpp"
#include "cpu.hpp"

namespace Sim {

class Simulator {
private:
    CPU cpu_;
    Register entry_point_;

public:
    Simulator()
        : cpu_(),
        entry_point_(0)
    {
        cpu_.reset();
    }

    ~Simulator() = default;

    CPU get_cpu() const{
        return cpu_;
    }

    bool load_program(const std::string &file_path) {
        return cpu_.load_program(file_path, 0);
    }

    void set_register(Register index, uint32_t value) {
        cpu_.set_register(index, value);
    }

    void set_pc(Address address) {
        cpu_.set_PC(address);
    }

    void write_memory(Address addr, uint32_t value) {
        if ((addr % kInstructionBytes) != 0) {
            std::cerr << "write_memory: unaligned address 0x" << std::hex << addr << std::dec << "\n";
            return;
        }
        cpu_.write(addr, value);
    }

    void run() {
        cpu_.run();
    }

    void dump_final_state() const {
        std::cout << "\n--- Simulation Finished ---\n";
        cpu_.dump_regs();
    }
};

} // namespace Sim

#endif //SIMULATOR_HPP_