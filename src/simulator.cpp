#include "simulator.hpp"
#include "config.hpp"

#include <iostream>
#include <filesystem>
#include <string>
#include <charconv>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <program.bin>\n x1=N";
        return 1;
    }

    std::filesystem::path program_path = argv[1];

    Sim::Simulator simulator;

    if (!simulator.load_program(program_path.string())) {
        std::cerr << "Failed to load program: " << program_path << "\n";
        return 1;
    }

    for (int i = 2; i < argc; ++i) {
        std::string arguments = argv[i];

        size_t eq_pos = arguments.find('=');
        if (eq_pos == std::string::npos
            || eq_pos < 2
            || (arguments[0] != 'x' && arguments[0] != 'X')) {
            std::cerr << "Invalid register argument: " << arguments << ". Expected format: x1=N\n";
            return 1;
        }

        std::string reg_str = arguments.substr(1, eq_pos - 1);

        size_t reg_idx = 0;
        char* reg_first = reg_str.data();
        char* reg_last = reg_str.data() + reg_str.size();

        std::from_chars_result rc_reg = std::from_chars(reg_first, reg_last, reg_idx, Sim::kBaseOfNumSys10);
        if (rc_reg.ec != std::errc()) {
            std::cerr << "Invalid register numder in: " << arguments << "\n";
            return 1;
        }
        if (static_cast<Sim::Register>(reg_idx) >= Sim::kNumberOfRegisters) {
            std::cerr << "Register index out of range (0.." << (Sim::kNumberOfRegisters - 1) << "): " << reg_idx << "\n";
            return 1;
        }

        std::string val_str = arguments.substr(eq_pos + 1);
        if (val_str.empty()) {
            std::cerr << "Empty value in argument: " << arguments << "\n";
            return 1;
        }

        size_t val_val = 0;
        std::from_chars_result rc_val;
        if (val_str.size() > 2 && (val_str[0] == '0')
            && (val_str[1] == 'x'
            || val_str[1] == 'X')) {
            rc_val = std::from_chars(val_str.data() + 2, val_str.data() + val_str.size(), val_val, Sim::kBaseOfNumSys16);
        } else {
            rc_val = std::from_chars(val_str.data(), val_str.data() + val_str.size(), val_val, Sim::kBaseOfNumSys10);
        }
        if (rc_val.ec != std::errc()) {
            std::cerr << "Bad number in argument: " << arguments << "\n";
            return 1;
        }

        Sim::Register value = static_cast<uint32_t>(val_val & 0xFFFFFFFF);
        simulator.set_register(reg_idx, value);
    }

    std::cout << "Starting simulation for '" << program_path << "'...\n";
    simulator.run();
    simulator.dump_final_state();
    return 0;
}