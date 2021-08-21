#include "kernel.h"
#include "ktypes.h"
#include "ecall.h"
#include "sched.h"
#include "io.h"
#include "malloc.h"

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
    char msg[28] = "found bin with id 0 at pos 0";
    ProcessControlBlock* next_process = processes;

    malloc_info info;
    info.allocate_memory_end = (void*) 0xFF0000;
    info.allocate_memory_start = (void*) 0;

    for (int i = 0; i < NUM_BINARIES; i++) {
        if (binary_table[i].binid == 0)
            break;
        // print message
        msg[18] = (char) binary_table[i].binid + '0';
        msg[27] = (char) i + '0';
        dbgln(msg, 28);
        info.allocate_memory_start = binary_table[i].bounds[1];
    }

    // initialize malloc
    malloc_init(&info);

    for (int i = 0; i < NUM_BINARIES; i++) {
        if (binary_table[i].binid == 0)
            break;


        optional_voidptr stack_top = malloc_stack(1<<12); // allocate 4Kib stack
        if (has_error(stack_top)) {
            dbgln("Error while allocating stack for initial process", 48);
            continue;
        }

        next_process->status = PROC_RDY;
        next_process->pid = binary_table[i].binid;
        next_process->pc = binary_table[i].entrypoint;

        next_process->regs[1] = (int) stack_top.value; // set stack top, put 32 bytes of zeros there
        next_process++;
        dbgln("enabled process from table", 26);

    }
}

