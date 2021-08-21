#ifndef H_KERNEL
#define H_KERNEL

#define true 1
#define false 0

#define XLEN          32            // 32 bit system
#define PROCESS_COUNT 32            // number of concurrent processes
#define NUM_BINARIES  16            // number of binaries loaded simultaneously 

// scheduler settings
#define TIME_SLICE_LEN 100          // number of cpu time ticks per slice

// init function
extern __attribute__((__noreturn__)) void init();

#endif