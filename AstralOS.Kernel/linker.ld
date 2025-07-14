ENTRY(_start)

SECTIONS {
    . = 0x1000;  /* where your kernel starts */
    _kernel_start = .;

    .text : ALIGN(4K) {
        KEEP(*(.text._start))
        *(.text*)
    }

    .rodata : ALIGN(4K) {
        *(.rodata*)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .bss : ALIGN(4K) {
        *(.bss*)
        *(COMMON)

        _stack_start = .;
        . = . + 0x4000;
        _stack_end = .;
    }

    .isr_stubs ALIGN(4K) : {
        KEEP(*(.isr_stubs))
    }

    _kernel_end = .;
}
