#ifndef H_SCHED
#define H_SCHED

#include "../kernel.h"

enum process_status {
    PROC_DEAD       = 0,
    PROC_RDY        = 1,
    PROC_WAIT_PROC  = 2,
    PROC_WAIT_SLEEP = 3,
};

// process structure:
typedef struct ProcessControlBlock ProcessControlBlock;
struct ProcessControlBlock {
    int pid;
    int pc;
    int regs[31];
    // scheduling information
    enum process_status status;
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
ProcessControlBlock* get_current_process();

#endif