#ifndef H_SCHED
#define H_SCHED

#include "../kernel.h"
#include "ktypes.h"

// scheduling data:
extern ProcessControlBlock processes[PROCESS_COUNT];

// scheduler methods
void scheudler_init();
ProcessControlBlock* scheduler_select_free();
void set_next_interrupt();
void __attribute__((noreturn)) scheduler_run_next();
void __attribute__((noreturn)) scheduler_try_return_to(ProcessControlBlock*);
void __attribute__((noreturn)) scheduler_switch_to(ProcessControlBlock*);
ProcessControlBlock* process_from_pid(int pid);
int* get_current_process_registers();
ProcessControlBlock* get_current_process();
void mark_ecall_entry();

// process creation / destruction
optional_pcbptr create_new_process(loaded_binary*, int);
optional_pcbptr create_new_thread(ProcessControlBlock*, void*, void*, int);
void destroy_process(ProcessControlBlock* pcb);
void kill_child_processes(ProcessControlBlock* pcb);
#endif
