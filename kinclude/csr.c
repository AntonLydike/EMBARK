#include "csr.h"

#ifdef TIMECMP_IN_MEMORY

void write_mtimecmp(unsigned long long int mtimecmp) {
    unsigned int lower = mtimecmp;
    unsigned int higher = mtimecmp >> 32;
    __asm__(
        "sw %1, 0(%0)\n"
        "sw %2, 4(%0)" :: 
        "r"(TIMECMP_MEM_ADDR),
        "r"(lower), "r"(higher)
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
        "csrr %0, 0xC01\n"
        "csrr %1, 0xC81\n"
        : "=r"(lower), "=r"(higher)
    );
    return (unsigned long long) higher << 32 | lower;
}
