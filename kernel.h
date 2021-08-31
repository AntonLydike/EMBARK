#ifndef H_KERNEL
#define H_KERNEL

// scheduler settings
#define TIME_SLICE_LEN 10          // number of cpu time ticks per slice

// set size of allocated stack for user processes
#define USER_STACK_SIZE (1 << 12)

// init function
extern __attribute__((__noreturn__)) void init();

#endif
