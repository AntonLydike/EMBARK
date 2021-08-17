OUTPUT_ARCH("riscv")
ENTRY(_start)
SECTIONS
{
    /* start at an address > 12bit to ensure all addresses are loaded using auipc+addi instead of only addi (which is not relocatable)*/
    . = 0xffffff; 
    .text : 
    {
        *(.text._start)
        *(.text)
    } 
    .sdata : 
    {
        __global_pointer$ = . + 0x800;
        *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata .srodata.*)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
    }
    _edata = .; PROVIDE (edata = .);
    . = .;
    __bss_start = .;
    .sbss :
    {
        *(.dynsbss)
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.scommon)
    }
    __bss_end = .;
    .stack : 
    {
        *(.stack)
    }
}