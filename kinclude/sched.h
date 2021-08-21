#ifndef H_SCHED
#define H_SCHED

#include "../kernel.h"
#include "ktypes.h"

// scheduling data:
extern ProcessControlBlock processes[PROCESS_COUNT];

// scheduler methods
ProcessControlBlock* scheduler_select_free();
int  scheduler_create_process(int binid);
void set_next_interrupt();
void __attribute__((noreturn)) scheduler_run_next();
void __attribute__((noreturn)) scheduler_try_return_to(ProcessControlBlock*);
void __attribute__((noreturn)) scheduler_switch_to(ProcessControlBlock*);
int  scheduler_index_from_pid(int pid);
int* get_current_process_registers();
ProcessControlBlock* get_current_process();
void mark_ecall_entry();
#endif