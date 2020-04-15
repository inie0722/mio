.global create_context
create_context:
//保存原协程信息
    push %rax
    push %rbx
    push %rcx

    mov %rsp, %rax
    mov %rbp, %rbx

//切换到新协程
    mov (%rdi), %rsp
    mov (%rdi), %rbp

    lea __start(%rip), %rcx
//入口地址
    push %rcx

//创建上下文
    push %rbp
    
    push %rsi
    push %rcx

    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, (%rdi)

//恢复 原协程
    mov %rax, %rsp
    mov %rbx, %rbp

    pop %rcx
    pop %rbx
    pop %rax

    ret

__start:
    mov %rax, %rdi
    call *%rbx

.global swap_context
swap_context:
    push %rbp

    push %rbx
    push %rcx

    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, (%rdi)
    mov %rsi, %rsp
    mov %rdx, %rax

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8

    pop %rcx
    pop %rbx

    pop %rbp
    
    ret
