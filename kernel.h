#ifndef H_KERNEL
#define H_KERNEL

#define true 1
#define false 0

#define XLEN          32            // 32 bit system
#define PROCESS_COUNT 32            // number of concurrent processes
#define NUM_BINARIES  16            // number of binaries loaded simultaneously 

// scheduler settings
#define TIME_SLICE_LEN 100          // number of cpu time ticks per slice


/* This struct holds information about binaries which are currently loaded into
 * memory. Currently the kernel is not able to load binaries into memory, as
 * no file system layer is implemented. When the memory image is built, the 
 * list of loaded binaries is populated aswell.
 */
typedef struct loaded_binary {
    int binid;
    int entrypoint;
    int bounds[2];
} loaded_binary;

// create a global table holding all loaded binaries.
// this is either populated at runtime when binaries are loaded dynamically
// or when a memory image is created.
extern loaded_binary binary_table[NUM_BINARIES];

// init function
extern __attribute__((__noreturn__)) void init();

#endif