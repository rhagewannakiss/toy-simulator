class Assembler
    OPCODE = {
        j:       0b100111,
        syscall: 0b000000,
        stp:     0b001000,
        rori:    0b111011,
        slti:    0b010110,
        st:      0b000011,
        bdep:    0b000000,
        cls:     0b000000,
        add:     0b000000,
        bne:     0b100100,
        beq:     0b000010,
        ld:      0b011010,
        and_:    0b000000,
        ssat:    0b001111,
    }.freeze

    SUB_ENCODING = {
        syscall: 0b111110,
        bdep:    0b111010,
        cls:     0b111001,
        add:     0b101011,
        and_:    0b101101,
    }.freeze

    REG_ALIAS = {
        "zero" => 0,
        "sp"   => 2,
        "ra"   => 1
    }.freeze

    private_constant :OPCODE, :SUB_ENCODING, :REG_ALIAS

    attr_reader :pc, :encoded

    def initialize(out_file: "out.bin")
        @out_file = out_file
        @pc = 0
        @encoded = []
        @labels = {}
        @lines = []
    end

    def assemble_from_file(file_path)
        @lines = File.readlines(file_path)

        # PASS 1
        current_address = 0
        @lines.each do |line|
            clean_line = line.split(';', 2).first.to_s.strip
            next if clean_line.empty?


            if m = clean_line.match(/\A([A-Za-z_]\w*):(?:\s*(.*))?\z/)
                label = m[1]
                rest = m[2]
                @labels[label] = current_address
                if rest && !rest.strip.empty?
                    current_address += 4
                end
            else
                current_address += 4
            end
        end

        # PASS 2
        @pc = 0
        @lines.each do |line|
            clean_line = line.split(';', 2).first.to_s.strip
            next if clean_line.empty?

            if m = clean_line.match(/\A([A-Za-z_]\w*):(?:\s*(.*))?\z/)
                instr_text = m[2]
                next if instr_text.nil? || instr_text.strip.empty?
                clean_line = instr_text.strip
            end

            if clean_line =~ /\A([#]?\-?0x[0-9a-fA-F]+|[#]?\-?\-?\d+)\z/
                val = to_int($1)
                @encoded << (val & 0xFFFFFFFF)
                @pc += 4
                next
            end

            parts = clean_line.split(/\s+/, 2)
            instr = parts[0].downcase
            args_str = parts[1] || ""
            args = args_str.split(',').map { |a| a.strip }.reject { |a| a.empty? }

            instr = 'and_' if instr == 'and'

            if ['ld', 'st', 'stp'].include?(instr)
                if !args.empty?
                    last = args.pop
                    if m2 = last.match(/\A(.+?)\((\w+)\)\z/)
                        offset_expr = m2[1].strip
                        base_reg = m2[2].strip
                        args.push(offset_expr, base_reg)
                    else
                        base_reg = last
                        args.push("0", base_reg)
                    end
                end
            end

            resolved_args = resolve_labels(instr, args)
            final_args = resolved_args.map do |a|
                if a.is_a?(String) && @labels.key?(a)
                    @labels[a]
                else
                    a
                end
            end

            begin
                self.send(instr.to_sym, *final_args)
            rescue NoMethodError
                raise "Unknown instruction '#{instr}' at pc=0x#{@pc.to_s(16)} (line: #{clean_line})"
            end
        end
    end

    # J target
    def j(target)
        target = to_int(target)
        raise "J target must be word-aligned: #{target}" unless (target & 0b11) == 0

        instr_index = (target >> 2) & 0x03FFFFFF
        word = (OPCODE[:j] << 26) | instr_index
        push_encoded(word)
    end

    # SYSCALL
    def syscall(code = 0)
        code = to_int(code)
        word = (OPCODE[:syscall] << 26) | ((code & 0x3FFFF) << 6) | SUB_ENCODING[:syscall]
        push_encoded(word)
    end

    # STP rt1, rt2, offset(base)
    def stp(rt1, rt2, offset, base)
        rt1_idx = reg_idx(rt1)
        rt2_idx = reg_idx(rt2)
        base_adr = reg_idx(base)
        offset_adr = to_int(offset)

        raise "STP offset must be word aligned: #{offset_adr}" unless (offset_adr & 0b11) == 0

        parsed_offset = offset_adr & 0x7FF
        word = (OPCODE[:stp] << 26) | (base_adr << 21) | (rt1_idx << 16) | (rt2_idx << 11) | parsed_offset
        push_encoded(word)
    end

    # RORI rd, rs, #imm5
    def rori(rd, rs, imm5)
        rd_idx = reg_idx(rd)
        rs_idx = reg_idx(rs)
        imm5_val = to_int(imm5) & 0x1F

        word = (OPCODE[:rori] << 26) | (rd_idx << 21) | (rs_idx << 16) | (imm5_val << 11)
        push_encoded(word)
    end

    # SLTI rt, rs, #imm
    def slti(rt, rs, imm)
        rt_idx = reg_idx(rt)
        rs_idx = reg_idx(rs)
        imm_val = to_int(imm) & 0xFFFF

        word = (OPCODE[:slti] << 26) | (rs_idx << 21) | (rt_idx << 16) | imm_val
        push_encoded(word)
    end

    # ST rt, offset(base)
    def st(rt, offset, base)
        rt_idx = reg_idx(rt)
        base_adr = reg_idx(base)
        offset_adr = to_int(offset)

        raise "ST offset must be word aligned: #{offset_adr}" unless (offset_adr & 0b11) == 0

        parsed_offset = (offset_adr & 0xFFFF)
        word = (OPCODE[:st] << 26) | (base_adr << 21) | (rt_idx << 16) | parsed_offset
        push_encoded(word)
    end

    # BDEP rd, rs1, rs2
    def bdep(rd, rs1, rs2)
        rd_idx = reg_idx(rd)
        rs1_idx = reg_idx(rs1)
        rs2_idx = reg_idx(rs2)

        word = (OPCODE[:bdep] << 26) | (rd_idx << 21) | (rs1_idx << 16) | (rs2_idx << 11) | SUB_ENCODING[:bdep]
        push_encoded(word)
    end

    # CLS rd, rs
    def cls(rd, rs)
        rd_idx = reg_idx(rd)
        rs_idx = reg_idx(rs)

        word = (OPCODE[:cls] << 26) | (rd_idx << 21) | (rs_idx << 16) | SUB_ENCODING[:cls]
        push_encoded(word)
    end

    # ADD rd, rs, rt
    def add(rd, rs, rt)
        rd_idx = reg_idx(rd)
        rs_idx = reg_idx(rs)
        rt_idx = reg_idx(rt)

        word = (OPCODE[:add] << 26) | (rs_idx << 21) | (rt_idx << 16) | (rd_idx << 11) | SUB_ENCODING[:add]
        push_encoded(word)
    end

    # BNE rs, rt, #offset
    def bne(rs, rt, offset)
        rs_idx = reg_idx(rs)
        rt_idx = reg_idx(rt)
        offset_val = to_int(offset) & 0xFFFF

        word = (OPCODE[:bne] << 26) | (rs_idx << 21) | (rt_idx << 16) | offset_val
        push_encoded(word)
    end

    # BEQ rs, rt, #offset
    def beq(rs, rt, offset)
        rs_idx = reg_idx(rs)
        rt_idx = reg_idx(rt)
        offset_val = to_int(offset) & 0xFFFF

        word = (OPCODE[:beq] << 26) | (rs_idx << 21) | (rt_idx << 16) | offset_val
        push_encoded(word)
    end

    # LD rt, offset(base)
    def ld(rt, offset, base)
        rt_idx = reg_idx(rt)
        base_adr = reg_idx(base)
        offset_adr = to_int(offset)

        raise "LD offset must be word aligned: #{offset_adr}" unless (offset_adr & 0b11) == 0

        parsed_offset = (offset_adr & 0xFFFF)
        word = (OPCODE[:ld] << 26) | (base_adr << 21) | (rt_idx << 16) | parsed_offset
        push_encoded(word)
    end

    # AND rd, rs, rt
    def and_(rd, rs, rt)
        rd_idx = reg_idx(rd)
        rs_idx = reg_idx(rs)
        rt_idx = reg_idx(rt)

        word = (OPCODE[:and_] << 26) | (rs_idx << 21) | (rt_idx << 16) | (rd_idx << 11) | SUB_ENCODING[:and_]
        push_encoded(word)
    end

    # SSAT rd, rs, #imm5
    def ssat(rd, rs, imm5)
        rd_idx = reg_idx(rd)
        rs_idx = reg_idx(rs)
        imm5_val = to_int(imm5)

        word = (OPCODE[:ssat] << 26) | (rd_idx << 21) | (rs_idx << 16) | ((imm5_val & 0x1F) << 11)
        push_encoded(word)
    end

    def write_file(path = nil)
        path ||= @out_file
        File.open(path, "wb") do |f|
            @encoded.each { |w| f.write([w].pack("V")) }
        end
        path
    end

    def dump
        puts "Label Symbol Table:"
        @labels.each { |name, addr| puts "  #{name}: 0x#{addr.to_s(16)}" }
        puts "\nEncoded Instructions:"
        @encoded.each_with_index do |w, i|
            puts sprintf("%04d (pc=0x%08X): 0x%08X", i, (i * 4), w)
        end
    end

private

    def resolve_labels(instruction, args)
        return [] if args.empty? || (args.size == 1 && args[0].empty?)

        if ['beq', 'bne'].include?(instruction)
            label = args.last
            raise "Undefined label: #{label}" unless @labels.key?(label)

            target_address = @labels[label]
            offset = (target_address - @pc) >> 2
            return args[0..-2] + [offset]
        end

        if instruction == 'j'
            label = args.first
            raise "Undefined label: #{label}" unless @labels.key?(label)
            return [@labels[label]]
        end

        args
    end

    def push_encoded(word)
        @encoded.push(word & 0xFFFFFFFF)
        @pc += 4
        word
    end

    def reg_idx(reg)
        s = reg.to_s.downcase
        return REG_ALIAS[s] if REG_ALIAS.key?(s)

        if s =~ /^[xr]?(\d{1,2})$/
            idx = $1.to_i
            raise "Register out of range: #{reg}" unless idx.between?(0, 31)
            return idx
        end

        raise "Bad register name: #{reg.inspect}"
    end

    def to_int(x)
        return x if x.is_a?(Integer)

        s = x.to_s.strip

        return s.to_i(16) if s.start_with?("0x")
        return s[1..-1].to_i if s.start_with?("#")
        s.to_i
    end
end