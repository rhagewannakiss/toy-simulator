#include "simulator.hpp"

#include <iostream>
#include <string>
#include <vector>

static bool parse_set_reg(const std::string &arg, Sim::Simulator &sim);
static bool parse_command_line_args(int argc, char *argv[], Sim::Simulator &sim, std::string &program_path);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program.bin> [--set-reg INDEX=VALUE] [--pc START_ADDRESS]\n";
        std::cerr << "Example: " << argv[0] << " fib.bin --set-reg 0=10 --set-reg 1=0 --set-reg 2=1\n";
        return 1;
    }

    Sim::Simulator simulator;
    std::string program_path;

    if (!parse_command_line_args(argc, argv, simulator, program_path)) {
        return 1;
    };

    if (!simulator.load_program(program_path)) {
        std::cerr <<"Failed tp load programm: " << program_path << "\n";
        return 1;
    }

    std::cout << "Starting simulation for '" << program_path << "'...\n";
    simulator.run();

    simulator.dump_final_state();

    return 0;
}


static bool parse_set_reg(const std::string &arg, Sim::Simulator &sim) {
    std::string s = arg;
    size_t eq = s.find('=');

    if (eq == std::string::npos) return false;
    try {
        unsigned long idx = std::stoul(s.substr(0, eq), nullptr, 0);
        unsigned long val = std::stoul(s.substr(eq + 1), nullptr, 0);
        sim.set_register(static_cast<uint32_t>(idx), static_cast<uint32_t>(val));
    } catch (...) {
        return false;
    }

    return true;
}

static bool parse_command_line_args(int argc, char *argv[], Sim::Simulator &sim, std::string &program_path) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.rfind("--set-reg=", 0) == 0) {
            std::string kv = arg.substr(10);
            if (!parse_set_reg(kv, sim)) {
                std::cerr << "Invalid format for --set-reg. Use --set-reg=INDEX=VALUE or --set-reg INDEX=VALUE\n";
                return false;
            }
            continue;
        }

        if (arg == "--set-reg") {
            if (i + 1 >= argc) {
                std::cerr << "Missing argument for --set-reg\n";
                return false;
            }
            ++i;
            if (!parse_set_reg(argv[i], sim)) {
                std::cerr << "Invalid format for --set-reg. Use --set-reg=INDEX=VALUE or --set-reg INDEX=VALUE\n";
                return false;
            }
            continue;
        }

        if (arg.rfind("--pc=", 0) == 0) {
            std::string pc_val = arg.substr(5);
            try {
                uint32_t addr = static_cast<uint32_t>(std::stoul(pc_val, nullptr, 0));
                sim.set_pc(addr);
            } catch (const std::exception &e) {
                std::cerr << "Error parsing PC address: " << e.what() << "\n";
                return false;
            }
            continue;
        }

        if (arg == "--pc") {
            if (i + 1 >= argc) {
                std::cerr << "Missing argument for --pc\n";
                return false;
            }
            ++i;
            try {
                uint32_t addr = static_cast<uint32_t>(std::stoul(argv[i], nullptr, 0));
                sim.set_pc(addr);
            } catch (const std::exception &e) {
                std::cerr << "Error parsing PC address: " << e.what() << "\n";
                return false;
            }
            continue;
        }

        if (!program_path.empty()) {
            std::cerr << "Error: Program path specified more than once.\n";
            return false;
        }
        program_path = arg;
    }

    if (program_path.empty()) {
        std::cerr << "Error: No program binary file specified.\n";
        return false;
    }

    return true;
}