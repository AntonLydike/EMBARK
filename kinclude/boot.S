#include "csr.h"
.section    .stack

stack_bottom:
.space      4096
stack_top:

.section    .text._start

.extern     init
.type       init, @function

.extern     trap_handle
.type       trap_handle, @function


.global     _start
_start:
            // setup a0 to hold |trap tbl addr|mode|
            //              len:| 30          | 2  |
            la      a0, trap_vector
            csrrw   zero, CSR_MTVEC, a0         // write a0 into mtvec csr entry
            // enable interrupts in mstatus
            // this is the setting loaded:
            // [07]    MPIE    =   1  - we want to enable interrupts with mret
            // [03]    MIE     =   0  - we don't want interrupts now
            // [11:12] MPP     =   0  - we want to return into user mode
            // all other bits should be zero
            li      a0, 0x80
            csrrw   zero, CSR_MSTATUS, a0         // write to mstatus
            // write
.option push
.option norelax
            // init sp and gp
            la      sp, stack_top
            la      gp, __global_pointer$
.option pop
            // clear kernel bss section
            mv      a0, zero
            la      a1, __bss_start
            la      a2, __bss_end
            jal     memset

            // jump to init
            jal     init
            
            // halt machine after returning from init
            csrwi   CSR_HALT, 1
1:
            j       1b

.align 4
trap_vector:
            // save all registers into the PCB struct
            // switch contents of t6 with contents of mscratch
            // mscratch holds the PCBs regs field address
            csrrw   t6,  mscratch, t6
            sw      ra,  0(t6)
            sw      sp,  4(t6)
            sw      gp,  8(t6)
            sw      tp,  12(t6)
            sw      t0,  16(t6)
            sw      t1,  20(t6)
            sw      t2,  24(t6)
            sw      s0,  28(t6)
            sw      s1,  32(t6)
            sw      a0,  36(t6)
            sw      a1,  40(t6)
            sw      a2,  44(t6)
            sw      a3,  48(t6)
            sw      a4,  52(t6)
            sw      a5,  56(t6)
            sw      a6,  60(t6)
            sw      a7,  64(t6)
            sw      s2,  68(t6)
            sw      s3,  72(t6)
            sw      s4,  76(t6)
            sw      s5,  80(t6)
            sw      s6,  84(t6)
            sw      s7,  88(t6)
            sw      s8,  92(t6)
            sw      s9,  96(t6)
            sw      s10, 100(t6)
            sw      s11, 104(t6)
            sw      t3,  108(t6)
            sw      t4,  112(t6)
            sw      t5,  116(t6)
            mv      a0,  t6             // save struct address to already saved register
            csrrw   t6,  mscratch, t6   // load original t6 register from mscratch
            sw      t6,  120(a0)        // save original t6 register
            csrr    a1,  mcause
            srli    a0,  a1, 31
            slli    a1,  a1, 1
            srli    a1,  a1, 1
            csrr    a2,  mtval
            // reinit sp and gp
.option push
.option norelax
            la      sp, stack_top
            la      gp, __global_pointer$
.option pop
            jal     trap_handle


#ifdef __risc_no_ext
// "dumb" memset, if RV32M is not present on the target
// since memset is currently only used at startup, the performance implications
// should be minimal.
memset:
    bge     a1, a2, 2f
1:
    sw      a0, 0(a1)
    addi    a1, a1, 4
    blt     a1, a2, 1b
2:
    ret

#else

// "smart" memset, writing 32 bytes at a time. uses RV32M. If not present,
// the "dumb" fallback above is used.

// write a0 to memory starting at a1, until a2 (both must be four byte aligned)
// this uses a loop which writes 32 (numbytes) bytes at a time
// to prevent overshooting the end, we first calulate how many instructions to 
// skip of the first iteration of the loop. this way, (a2 - a1) is a multiple of
// (numbytes) when we reach the blt instruction for the first time. 
// this math works so good, because we write 4 bytes of mem, in 4 bytes of 
// instructions. Therefore instruction bytes to skip = write bytes to skip
// bytes to skip = numbytes - ((a2 - a1) % numbytes)
memset:
            sub     t1, a2, a1      // t1 = a2 - a1
            li      t2, 32          // = numbytes
            rem     t1, t1, t2      // t1 = (a2 - a1) % numbytes
            beq     zero, t1, 1f    // skip 0 bytes? => begin loop
            sub     t2, t2, t1      // t2 = numbytes - ((a2 - a1) % numbytes)
                                    // = bytes to skip
            sub     a1, a1, t2      // subtract skipped bytes from a2
                                    // to account for the skipped instruction
                                    // when we reach the addi, a1, a1, 32 inst.
            auipc   t1, 0           // get current address
            add     t1, t2, t1      // add calulated offset
            jalr    zero, t1, 12    // skip the instructions by forward-jumping
                                    // the 12 is added to compensate for the 
                                    // three instructions auipc, add, jalr
1:
            sw      a0, 0(a1)
            sw      a0, 4(a1)
            sw      a0, 8(a1)
            sw      a0, 12(a1)
            sw      a0, 16(a1)
            sw      a0, 20(a1)
            sw      a0, 24(a1)
            sw      a0, 28(a1)
            addi    a1, a1, 32
            blt     a1, a2, 1b
            ret
#endif