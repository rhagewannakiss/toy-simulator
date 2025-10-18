require_relative 'assembler'

if ARGV.length != 2
  puts "Usage: ruby run_assembler.rb <input_file.asm> <output_file.bin>"
  exit 1
end

input_file = ARGV[0]
output_file = ARGV[1]

unless File.exist?(input_file)
  puts "Error: Input file '#{input_file}' not found."
  exit 1
end

assembler = Assembler.new(out_file: output_file)

assembler.assemble_from_file(input_file)
assembler.dump
assembler.write_file