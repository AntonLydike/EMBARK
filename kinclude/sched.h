#ifndef H_SCHED
#define H_SCHED

#include "../kernel.h"
#include "ktypes.h"

// scheduling data:
extern struct process_control_block processes[PROCESS_COUNT];

// scheduler methods
void scheudler_init();
struct process_control_block* scheduler_select_free();
void set_next_interrupt();
void __attribute__((noreturn)) scheduler_run_next();
void __attribute__((noreturn)) scheduler_try_return_to(struct process_control_block*);
void __attribute__((noreturn)) scheduler_switch_to(struct process_control_block*);
struct process_control_block* process_from_pid(int pid);
int* get_current_process_registers();
struct process_control_block* get_current_process();
void mark_ecall_entry();

// process creation / destruction
optional_pcbptr create_new_process(loaded_binary*);
optional_pcbptr create_new_thread(struct process_control_block*, void*, void*);
void destroy_process(struct process_control_block* pcb);
void kill_child_processes(struct process_control_block* pcb);
#endif
