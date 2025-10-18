#include "simulator.hpp"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program.bin> [--set-reg INDEX=VALUE] [--pc START_ADDRESS]\n";
        std::cerr << "Example: " << argv[0] << " fib.bin --set-reg 0=10 --set-reg 1=0 --set-reg 2=1\n";
        return 1;
    }

    Sim::Simulator simulator;
    std::string program_path;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.rfind("--set-reg=", 0) == 0) {
            std::string reg_val = arg.substr(10);
            size_t separator_pos = reg_val.find('=');
            if (separator_pos == std::string::npos) {
                std::cerr << "Invalid format for --set-reg. Use --set-reg=INDEX=VALUE or --set-reg INDEX=VALUE\n";
                return 1;
            }
            try {
                uint32_t index = std::stoul(reg_val.substr(0, separator_pos));
                uint32_t value = std::stoul(reg_val.substr(separator_pos + 1));
                simulator.set_register(index, value);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing register value: " << e.what() << "\n";
                return 1;
            }
        } else if (arg == "--set-reg") {
            if (i + 1 >= argc) {
                std::cerr << "Missing argument for --set-reg\n";
                return 1;
            }
            std::string reg_val = argv[++i];
            size_t separator_pos = reg_val.find('=');
            if (separator_pos == std::string::npos) {
                std::cerr << "Invalid format for --set-reg. Use --set-reg=INDEX=VALUE or --set-reg INDEX=VALUE\n";
                return 1;
            }
            try {
                uint32_t index = std::stoul(reg_val.substr(0, separator_pos));
                uint32_t value = std::stoul(reg_val.substr(separator_pos + 1));
                simulator.set_register(index, value);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing register value: " << e.what() << "\n";
                return 1;
            }
        } else if (arg.rfind("--pc=", 0) == 0) {
            std::string pc_val = arg.substr(5);
            try {
                uint32_t addr = std::stoul(pc_val, nullptr, 0);
                simulator.set_pc(addr);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing PC address: " << e.what() << "\n";
                return 1;
            }
        } else if (arg == "--pc") {
            if (i + 1 >= argc) {
                std::cerr << "Missing argument for --pc\n";
                return 1;
            }
            std::string pc_val = argv[++i];
            try {
                uint32_t addr = std::stoul(pc_val, nullptr, 0);
                simulator.set_pc(addr);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing PC address: " << e.what() << "\n";
                return 1;
            }
        } else {
            if (!program_path.empty()) {
                std::cerr << "Error: Program path specified more than once.\n";
                return 1;
            }
            program_path = arg;
        }
    }

    if (program_path.empty()) {
        std::cerr << "Error: No program binary file specified.\n";
        return 1;
    }

    if (!simulator.load_program(program_path)) {
        std::cerr << "Failed to load program: " << program_path << "\n";
        return 1;
    }

    std::cout << "Starting simulation for '" << program_path << "'...\n";
    simulator.run();

    simulator.dump_final_state();

    return 0;
}



namespace Sim {

    bool Simulator::load_program(const std::string &file_path) {
        return cpu_.load_program(file_path, 0);
    }

    void Simulator::set_register(uint32_t index, uint32_t value) {
        cpu_.set_register(index, value);
    }

    void Simulator::set_pc(Address address) {
        cpu_.set_PC(address);
    }

    void Simulator::run() {
        cpu_.run();
    }

    void Simulator::dump_final_state() const {
        CPU temp_cpu = cpu_;
        std::cout << "\n--- Simulation Finished ---\n";
        temp_cpu.dump_regs();
    }

} // namespace Sim
