#include "ktypes.h"
#include "csr.h"

#ifdef TIMECMP_IN_MEMORY

#ifndef TIMECMP_MEM_ADDR
#error "You set TIMECMP_IN_MEMORY but did not provide a memory addres in TIMECMP_MEM_ADDR!"
#endif

void write_mtimecmp(uint64 mtimecmp)
{
    uint32 lo = mtimecmp & 0xffffffff;
    uint32 hi = mtimecmp >> 32;

    __asm__ volatile (
          "li t0, %0\n"
          "sw %1, 0(t0)\n"
          "sw %2, 4(t0)" ::
          "i"(TIMECMP_MEM_ADDR), "r"(lo), "r"(hi)
    );
}

#else

void write_mtimecmp(uint64 mtimecmp)
{
    uint32 lower = mtimecmp & 0xffffffff;
    uint32 higher = mtimecmp >> 32;

    __asm__ volatile (
          "csrw %0, %2\n"
          "csrw %1, %3" ::
          "I"(CSR_MTIMECMP),"I"(CSR_MTIMECMPH),
          "r"(lower), "r"(higher)
    );
}

#endif
