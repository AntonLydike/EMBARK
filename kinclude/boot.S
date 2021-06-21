.section    .stack

stack_bottom:
.space      4096
stack_top:


.section    .text


// Set up all the CSR mstatus_offsets
.set        mstatus,    0x300       // machine status
.set        mscratch,   0x340
.set        mtvec,      0x305       // machine trap handler
.set        mcause,     0x342       // machine trap cause
.set        mtval,      0x343       // machine bad address or instruction
// .set        misa,       0x301       // machine ISA
// .set        mie,        0x304       // machine interrupt enable
// .set        mip,        0x344       // machine interrupt pending

.extern     init
.type       init, @function

.extern     trap_handle
.type       trap_handle, @function


.global     _start
_start:
            // setup a0 to hold |trap tbl addr|mode|
            //              len:| 30          | 2  |
            la      a0, trap_vector
            csrrw   zero, mtvec, a0         // write a0 into mtvec csr entry
            // enable interrupts in mstatus
            // this is the setting loaded:
            // [07]    MPIE    =   1  - we want to enable interrupts with mret
            // [03]    MIE     =   0  - we don't want interrupts now
            // [11:12] MPP     =   0  - we want to return into user mode
            // all other bits should be zero
            li      a0, 0x80
            csrrw   zero, mstatus, a0         // write to mstatus
            // write
.option push
.option norelax
            // init sp and gp
            la      sp, stack_top
            la      gp, __global_pointer$
.option pop
            jal     init
            
            // halt machine

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
