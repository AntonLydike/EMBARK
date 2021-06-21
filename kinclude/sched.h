#ifndef H_SCHED
#define H_SCHED

#include "../kernel.h"

// process statuses:
#define PROC_DEAD 0
#define PROC_RDY  1
#define PROC_WAIT_LOCK  2
#define PROC_WAIT_PROC  3
#define PROC_WAIT_SLEEP 4

// process structure:
typedef struct ProcessControlBlock ProcessControlBlock;
struct ProcessControlBlock {
    int pid;
    int pc;
    int regs[31];
    // scheduling information
    int status;
    int requested_lock;
    ProcessControlBlock *waiting_for_process;
    unsigned long long int asleep_until;
};

// scheduling data:
extern ProcessControlBlock processes[PROCESS_COUNT];

// scheduler methods
int  scheduler_select_free();
int  scheduler_create_process(int binid);
void __attribute__((noreturn)) scheduler_run_next();
void __attribute__((noreturn)) scheduler_switch_to(int proc_index);
int  scheduler_index_from_pid(int pid);
int* get_current_process_registers();

#endif