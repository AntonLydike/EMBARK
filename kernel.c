#include "kernel.h"
#include "ecall.h"
#include "sched.h"
#include "io.h"

void create_processes_from_bin_table();

extern ProcessControlBlock processes[PROCESS_COUNT];

loaded_binary binary_table[NUM_BINARIES] __attribute__ ((section (".data")));

extern void memset(unsigned int, void*, void*);

extern void init()
{
    init_ecall_table();

    dbgln("Kernel started!", 15);

    create_processes_from_bin_table();

    scheduler_run_next();   
}

void create_processes_from_bin_table()
{

}

