#ifndef H_KERNEL
#define H_KERNEL

#define true 1
#define false 0

#define XLEN          32            // 32 bit system
#define MUTEX_COUNT   64            // must be multiple of xlen
#define PROCESS_COUNT 64
#define MAX_INT       0x7FFFFFFF    // max 32 bit signed int

// memory layout:
#define ROM_START 0x00100
#define IO_START  0x10000
#define NVM_START 0x20000
#define RAM_START 0x50000

// scheduler settings
#define TIME_SLICE_LEN 100          // number of cpu time ticks per slice

// init function
extern __attribute__((__noreturn__)) void init();

#endif