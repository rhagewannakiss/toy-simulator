# Functional TOY ISA v1.1 CPU Simulator

## About

This project is a functional simulator for the custom **TOY ISA v1.1** architecture. It was developed as a course project to demonstrate the principles of CPU emulation.

The simulator executes 32-bit machine code generated from a custom assembly language. It currently supports all instructions specified in the TOY ISA v1.1 documentation, including basic arithmetic, memory access, branching, and system calls for I/O and halting the machine.

The project includes two main components:
1.  An **assembler** written in Ruby, which translates TOY assembly code into a raw binary format.
2.  A **CPU simulator** written in C++, which loads and executes the binary programs.

## Installation

### Prerequisites
1.  A C++ compiler (g++ recommended)
2.  CMake (version 3.15 or later)
3.  Ruby (for the assembler script)

### Build Steps

Clone the repository (or simply navigate to your project directory if it's not a git repo):
```bash
# If you have a git repository:
git clone <your_repository_url>
cd <your_project_directory>
```
Configure and build the C++ simulator using CMake:

```bash
# Compile the project using all available CPU cores
cmake --build build --parallel `nproc`
```

After a successful build, the simulator executable will be located at build/toy_cpu.

## Usage
The workflow consists of two stages: assembling your program and running it on the simulator.

1. Assembling the Program
Use the provided Ruby script to convert your .asm source file into a .bin executable binary.

Command:

``` bash
ruby ./src/asm/run_assembler.rb <input_file.asm> <output_file.bin>
```
Example:

```Bash
ruby /src/asm/run_assembler.rb fib.asm fib.bin
```

This will create fib.bin in your project's root directory.

2. Running the Simulator
Execute the compiled simulator, providing the path to your binary file. You can also set initial register values and the starting program counter (PC) via command-line flags.

Command Syntax:

```bash
./build/toy_cpu <path_to_binary.bin> [x[reg_id]=[value]]
```
Example:
To run the fib.bin program to calculate fib(12), you need to initialize register x1:

```bash
./build/bin/toy_cpu fib.bin x1=12
```
The simulator will execute the program until a halt syscall is encountered and then print the final state of all CPU registers.