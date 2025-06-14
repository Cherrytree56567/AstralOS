ENTRY(_start)

SECTIONS {
    . = 0x1000;  /* where your kernel starts */
    _kernel_start = .;

    .text : ALIGN(4K) {
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
    }

    _kernel_end = .;
}
