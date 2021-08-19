#ifndef H_CSR
#define H_CSR

#define CSR_MSTATUS     0x300       // machine status
#define CSR_MISA        0x301       // machine ISA
#define CSR_MIE         0x304       // machine interrupt enable
#define CSR_MTVEC       0x305       // machine trap handler
#define CSR_MSCRATCH    0x340       // machine scratch register
#define CSR_MEPC        0x341       // machine exception program counter
#define CSR_MCAUSE      0x342       // machine trap cause
#define CSR_MTVAL       0x343       // machine bad address or instruction
#define CSR_MIP         0x344       // machine interrupt pending
#define CSR_TIME        0xC01       // time csr
#define CSR_TIMEH       0xC81       // high bits of time

#define CSR_MTIMECMP    0x780       // mtimecmp register for timer interrupts
#define CSR_MTIMECMPH   0x781       // mtimecmph register for timer interrupts

#ifndef CSR_HALT
#define CSR_HALT        0x789       // writing nonzero value here will halt the cpu
#endif

// do not define C macros and other C stuff when this is included inside assembly
#ifndef __assembly

#define CSR_READ(csr_id, result) {\
    __asm__ ("csrr %0, %1" : "=r"((result)) : "I"((csr_id))); \
}

#define CSR_WRITE(csr_id, val) {\
    __asm__ ("csrw %0, %1" :: "I"((csr_id)), "r"((val))); \
}

#define HALT(code) {\
    __asm__("csrw %0, %1" :: "I"(CSR_HALT), "I"(code)); \
}

void write_mtimecmp(unsigned long long int mtimecmp);

inline __attribute__((always_inline)) unsigned long long int read_time() {
    unsigned int lower, higher;
    __asm__(
        "csrr %0, %2\n"
        "csrr %1, %3\n"
        : "=r"(lower), "=r"(higher)
        : "i"(CSR_TIME), "i"(CSR_TIMEH)
    );
    return (unsigned long long) higher << 32 | lower;
}

#endif

#endif