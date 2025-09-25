; tell NASM we’re in 64‑bit mode
bits 64

section .isr_stubs

; code found in OSDev
; https://wiki.osdev.org/Interrupts_Tutorial
%macro isr_err_stub 1
isr_stub_%+%1:
    cli
    push rax
    push rcx
    push rdx
    push rbx
    push rsp
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    pushfq
    mov rdi, %1
    lea rdx, [rsp + 136]
    mov rsi, [rdx + 24]
    call exception_handler
    popfq
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rsp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    sti
    iretq
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:
    cli
    push rax
    push rcx
    push rdx
    push rbx
    push rsp
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    pushfq
    mov rdi, %1
    mov rsi, 0
    lea rdx, [rsp + 8]
    call exception_handler
    popfq
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rsp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    sti
    iretq
%endmacro

%macro isr_hardware_stub 1
isr_stub_%+%1:
    cli
    push rax
    push rcx
    push rdx
    push rbx
    push rsp
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    pushfq
    mov rdi, %1
    lea rdx, [rsp + 8]
    call hardware_handler
    popfq
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rsp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    sti
    iretq
%endmacro

extern exception_handler
extern hardware_handler
extern apic_handler
isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31
isr_hardware_stub 32
isr_hardware_stub 33
isr_hardware_stub 34
isr_hardware_stub 35
isr_hardware_stub 36
isr_hardware_stub 37
isr_hardware_stub 38
isr_hardware_stub 39
isr_hardware_stub 40
isr_hardware_stub 41
isr_hardware_stub 42
isr_hardware_stub 43
isr_hardware_stub 44
isr_hardware_stub 45
isr_hardware_stub 46
isr_hardware_stub 47

%assign start_vec 0x30
%assign end_vec 0xFF

%assign vec start_vec
%rep end_vec - start_vec + 1
    isr_stub_%+vec:
        mov rdi, vec
        call apic_handler
        iretq
%assign vec vec + 1
%endrep

global irq_stub
extern irq1_handler
irq_stub:
    push rax
    push rcx
    push rdx
    call irq1_handler
    pop rdx
    pop rcx
    pop rax
    iretq

global isr_stub_table
isr_stub_table:
%assign i 0 
%rep    256 
    dq isr_stub_%+i ; use DD instead if targeting 32-bit
%assign i i+1 
%endrep