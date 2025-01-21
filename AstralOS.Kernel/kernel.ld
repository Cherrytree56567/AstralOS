SECTIONS
{
    . = 0x100000;       /* Set the starting address for the .text section */
    kernel_start = .;   /* Mark the start of the kernel */

    .text : {
        *(.text)        /* Include all .text sections from input files */
    }

    .rodata : {
        *(.rodata)      /* Include all .rodata sections from input files */
    }

    .data : {
        *(.data)        /* Include all .data sections from input files */
    }

    .bss : {
        *(.bss)         /* Include all .bss sections from input files */
        *(COMMON)
    }

    kernel_end = .;     /* Mark the end of the kernel */
}
