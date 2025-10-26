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

if ARGV.length == 2
  output_file = ARGV[1]
else
  base = File.basename(input_file, File.extname(input_file))
  script_dir = __dir__
  project_root = File.expand_path('../../', script_dir)
  output_file = File.join(project_root, "#{base}.bin")
end

assembler = Assembler.new(out_file: output_file)

assembler.assemble_from_file(input_file)
assembler.dump
assembler.write_file
puts "Assembled #{assembler.encoded.size} instructions into #{output_file}"