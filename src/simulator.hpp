#ifndef SIMULATOR_HPP_
#define SIMULATOR_HPP_

#include "config.hpp"
#include "cpu.hpp"

namespace Sim {

class Simulator{
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

    bool load_program(const std::string &file_path);
    void set_register(uint32_t index, uint32_t value);
    void set_pc(Address address);
    void run();
    void dump_final_state() const;
};

} // namespace Sim

#endif //SIMULATOR_HPP_