#include "kernel.h"
#include "ktypes.h"
#include "ecall.h"
#include "sched.h"
#include "io.h"
#include "malloc.h"

void read_binary_table();

extern ProcessControlBlock processes[PROCESS_COUNT];

// this array is populated when the memory image is built, therefore it should
// resign in a section which is not overwritten with zeros on startup
loaded_binary binary_table[NUM_BINARIES] __attribute__ ((section(".data")));

extern void memset(unsigned int, void*, void*);

extern void init()
{
    init_ecall_table();

    dbgln("Kernel started!", 15);

    read_binary_table();

    scheduler_run_next();
}

void read_binary_table()
{
    char msg[28] = "found bin with id 0 at pos 0";

    malloc_info info;

    info.allocate_memory_end = (void*) 0xFF0000;
    info.allocate_memory_start = (void*) 0;

    // calculate the end of loaded binaries
    for (int i = 0; i < NUM_BINARIES; i++) {
        if (binary_table[i].binid == 0)
            break;

        if (DEBUGGING) {
            // print message
            msg[18] = (char) binary_table[i].binid + '0';
            msg[27] = (char) i + '0';
            dbgln(msg, 28);
        }

        info.allocate_memory_start = binary_table[i].bounds[1];
    }

    // initialize malloc
    malloc_init(&info);

    for (int i = 0; i < NUM_BINARIES; i++) {
        if (binary_table[i].binid == 0)
            break;

        // create a new process for each binary found
        // it should have around 4kb stack
        optional_pcbptr res = create_new_process(binary_table + i, 1 << 12);
        if (has_error(res)) {
            dbgln("Error creating initial process!", 31);
        }
    }
}
