; fib.asm -- computes fib(n)
; input: X0 = n
; section .data:
;   X1 = 0   ; a
;   X2 = 1   ; b
;   X4 = 2   ; i
;   X5 = 1   ; const 1

        ; if n == 0  print 0
        BEQ x0, x1, print_zero

        ; if n == 1  print 1
        BEQ x0, x2, print_one

loop:
        ; if i == n done (result in b = X2)
        BEQ x4, x0, done

        ; t = a + b
        ADD x3, x1, x2

        ; a = b
        SSAT x1, x2, #0

        ; b = t
        SSAT x2, x3, #0

        ; i = i + 1
        ADD x4, x4, x5

        ; if i != n jump back to loop
        BNE x4, x0, loop

done:
        ; move b x0 (result)
        SSAT x0, x2, #0
        SYSCALL #1        ; print X0
        SYSCALL #0        ; halt

print_zero:
        SSAT x0, x1, #0
        SYSCALL #1
        SYSCALL #0

print_one:
        SSAT x0, x2, #0
        SYSCALL #1
        SYSCALL #0