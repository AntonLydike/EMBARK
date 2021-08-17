#include "kernel.h"
#include "ecall.h"
#include "sched.h"

void thread_1();

extern ProcessControlBlock processes[PROCESS_COUNT];

loaded_binary binary_table[NUM_BINARIES];

static int idx = 0;

extern void init()
{
    for (int i = 0; i < 100; i++) {
        idx += binary_table[i].entrypoint + 4;
    }

    scheduler_run_next();   
}

