OUTPUT_FORMAT("elf64-upsilon")
OUTPUT_ARCH(upsilon)
ENTRY(_start)
SECTIONS {
 . = 0x200000;
 .text : { KEEP(*(.text.entry)) *(.text*) }
 .rodata ALIGN(4096) : { *(.rodata*) }
 .usertext ALIGN(4096) : { *(.usertext) }
 .forkusertext ALIGN(4096) : { *(.forkusertext) }
 .data ALIGN(4096) : { *(.data*) }
 .bss ALIGN(4096) : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; }
 /DISCARD/ : { *(.comment) *(.note.GNU-stack) *(.eh_frame*) }
}
