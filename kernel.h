#ifndef H_KERNEL
#define H_KERNEL

#define true 1
#define false 0

#define PROCESS_COUNT 8             // number of concurrent processes
#define NUM_BINARIES  4             // number of binaries loaded simultaneously

// scheduler settings
#define TIME_SLICE_LEN 10          // number of cpu time ticks per slice

// set size of allocated stack for user processes
#define USER_STACK_SIZE (1 << 12)

// init function
extern __attribute__((__noreturn__)) void init();

#endif
