start:
    ADD x2, x0, x0      ; a = 0
    SLTI x3, x0, #1     ; 1
    ADD x4, x0, x3      ; i = 1
    ADD x5, x3, x0      ; b = x5 = 1

    ; if x1 == 0
    BEQ x1, x2, print_zero
    ; if x1 == 1
    BEQ x1, x3, print_one

loop:
    ADD x6, x2, x5      ; x6 = a + b
    ADD x2, x5, x0      ; a = b
    ADD x5, x6, x0      ; b = a + b
    ADD x4, x4, x3      ; i++
    BNE x4, x1, loop    ; i != n

done:
    ADD x0, x5, x0
    SYSCALL #1
    SYSCALL #0

print_zero:
    ADD x0, x2, x0
    SYSCALL #1
    SYSCALL #0

print_one:
    ADD x0, x5, x0
    SYSCALL #1
    SYSCALL #0