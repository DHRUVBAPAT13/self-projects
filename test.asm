section .text
    global _start

    _start:
        mov rax,1
        mov rdi,1
        mov rsi,8
        mov rdx,1
        syscall

        mov rax,60
        mov rdi,0
        syscall