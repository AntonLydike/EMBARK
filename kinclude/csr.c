#include "csr.h"

#ifdef TIMECMP_IN_MEMORY

#ifndef TIMECMP_MEM_ADDR
#error "You set TIMECMP_IN_MEMORY but did not provide a memory addres in TIMECMP_MEM_ADDR!"
#endif

void write_mtimecmp(unsigned long long int mtimecmp) {
    unsigned int lo = mtimecmp;
    unsigned int hi = mtimecmp >> 32;
    __asm__(
        "li t0, %0\n"
        "sw %1, 0(t0)\n"
        "sw %2, 4(t0)" :: 
        "i"(TIMECMP_MEM_ADDR), "r"(lo), "r"(hi)
    );
}

#else

void write_mtimecmp(unsigned long long int mtimecmp) {
    unsigned int lower = mtimecmp;
    unsigned int higher = mtimecmp >> 32;
    __asm__(
        "csrw %0, %2\n"
        "csrw %1, %3" :: 
        "I"(CSR_MTIMECMP),"I"(CSR_MTIMECMPH),
        "r"(lower), "r"(higher)
    );
}

#endif


unsigned long long int read_time() {
    unsigned int lower, higher;
    __asm__(
        "csrr %0, %2\n"
        "csrr %1, %3\n"
        : "=r"(lower), "=r"(higher)
        : "i"(CSR_TIME), "i"(CSR_TIMEH)
    );
    return (unsigned long long) higher << 32 | lower;
}
