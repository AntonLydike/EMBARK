#include "../kernel.h"
#include "sched.h"
#include "csr.h"
#include "io.h"
#include "malloc.h"

// use memset provided in boot.S
extern void memset(int, void*, void*);
// this is the address where threads return to
// it's located in a seprate section at the end of the kernel
extern int thread_finalizer;

// process list, holds all active and some dead processes
struct process_control_block processes[PROCESS_COUNT];
// pointer to the currently scheduled process
struct process_control_block* current_process = NULL;
// timer variables to add kernel time back to the processes time slice
unsigned long long int scheduling_interrupted_start;
unsigned long long int next_interrupt_scheduled_for;
// this counter generates process ids
int next_process_id = 1;

// run the next process
void scheduler_run_next()
{
    current_process = scheduler_select_free();
    // set up timer interrupt
    set_next_interrupt();
    scheduler_switch_to(current_process);
}

void scheudler_init()
{
    current_process = processes + PROCESS_COUNT - 1;
}

// try to return to a process
void scheduler_try_return_to(struct process_control_block* pcb)
{
    // if the process isn't ready, schedule a new one
    if (pcb->status != PROC_RDY) {
        scheduler_run_next();
    } else {
        // if we want to return to the current process...
        if (current_process == pcb) {
            dbgln("returning to process...", 23);
            // add time spent in ecall handler to the processes time slice
            next_interrupt_scheduled_for = next_interrupt_scheduled_for + (read_time() - scheduling_interrupted_start);
            write_mtimecmp(next_interrupt_scheduled_for);
            scheduler_switch_to(current_process);
        } else {
            // otherwise set a new interrupt
            set_next_interrupt();
            current_process = pcb;
            scheduler_switch_to(current_process);
        }
    }
}

// select a new process to run next
struct process_control_block* scheduler_select_free()
{
    unsigned long long int mtime;
    int timeout_available = 0; // note if a timeout is available

    while (1) {
        mtime = read_time();
        // start at the last scheduled process
        struct process_control_block* pcb = current_process;

        // iterate once over the whole list
        do {
            // get next pcb
            pcb++;
            // wrap around the end of the list
            if (pcb > processes + PROCESS_COUNT)
                pcb = processes;

            // when we find a process which is ready to be scheduled, return it!
            if (pcb->status == PROC_RDY)
                return pcb;

            // if it's sleeping, check if it is time to wake it up
            if (pcb->status == PROC_WAIT_SLEEP) {
                if (pcb->asleep_until < mtime) {
                    pcb->status = PROC_RDY;
                    return pcb;
                }
                timeout_available = 1;
            }

            // if it's waiting for another process, check if the process exited
            // or if is waiting with a timeout, tell it the timeout expired
            if (pcb->status == PROC_WAIT_PROC) {
                if (pcb->waiting_for_process != NULL &&
                    pcb->waiting_for_process->status == PROC_DEAD) {
                    // the requested process exited, so we can set the status code and
                    pcb->regs[REG_A0] = pcb->waiting_for_process->exit_code;
                    pcb->regs[REG_A0 + 1] = 0;
                    pcb->status = PROC_RDY;
                    return pcb;
                }
                if (pcb->asleep_until != 0) {
                    if (pcb->asleep_until < mtime) {
                        // if the timeout ran out, set an error code
                        pcb->regs[REG_A0 + 1] = ETIMEOUT;
                        pcb->status = PROC_RDY;
                        return pcb;
                    }
                    timeout_available = 1;
                }
            }
        } while (pcb != current_process);

        // when we finished iterating over all processes and no process can be scheduled we have a problem
        if (timeout_available == 0) {
            // either process deadlock without timeout or no processes alive.
            //TODO: handle deadlocks by killing a process
            dbgln("No thread active!", 17);
            HALT(22);
        }
    }
}

void scheduler_switch_to(struct process_control_block* pcb)
{
    CSR_WRITE(CSR_MEPC, pcb->pc);

    // set up registers
    __asm__ (
         "mv     x31, %0\n"
         "csrrw  zero, %1, x31\n"
         "lw     x1,  0(x31)\n"
         "lw     x2,  4(x31)\n"
         "lw     x3,  8(x31)\n"
         "lw     x4,  12(x31)\n"
         "lw     x5,  16(x31)\n"
         "lw     x6,  20(x31)\n"
         "lw     x7,  24(x31)\n"
         "lw     x8,  28(x31)\n"
         "lw     x9,  32(x31)\n"
         "lw     x10, 36(x31)\n"
         "lw     x11, 40(x31)\n"
         "lw     x12, 44(x31)\n"
         "lw     x13, 48(x31)\n"
         "lw     x14, 52(x31)\n"
         "lw     x15, 56(x31)\n"
         "lw     x16, 60(x31)\n"
         "lw     x17, 64(x31)\n"
         "lw     x18, 68(x31)\n"
         "lw     x19, 72(x31)\n"
         "lw     x20, 76(x31)\n"
         "lw     x21, 80(x31)\n"
         "lw     x22, 84(x31)\n"
         "lw     x23, 88(x31)\n"
         "lw     x24, 92(x31)\n"
         "lw     x25, 96(x31)\n"
         "lw     x26, 100(x31)\n"
         "lw     x27, 104(x31)\n"
         "lw     x28, 108(x31)\n"
         "lw     x29, 112(x31)\n"
         "lw     x30, 116(x31)\n"
         "lw     x31, 120(x31)\n"
         "mret   \n"
         :: "r"(pcb->regs), "I"(CSR_MSCRATCH)
    );
    __builtin_unreachable();
}

struct process_control_block* process_from_pid(int pid)
{
    for (int i = 0; i < PROCESS_COUNT; i++) {
        if (processes[i].pid == pid)
            return processes + i;
    }
    return NULL;
}

int* get_current_process_registers()
{
    return current_process->regs;
}

struct process_control_block* get_current_process()
{
    return current_process;
}

void set_next_interrupt()
{
    next_interrupt_scheduled_for = read_time() + TIME_SLICE_LEN;
    write_mtimecmp(next_interrupt_scheduled_for);
}

void mark_ecall_entry()
{
    scheduling_interrupted_start = read_time();
}

optional_pcbptr find_available_pcb_slot()
{
    static int index = 0;
    int start_index = index;
    struct process_control_block* pcb = processes + index;

    while (pcb->status != PROC_DEAD) {
        index = (index + 1) % PROCESS_COUNT;
        if (index == start_index)
            return (optional_pcbptr) { .error = ENOBUFS };
        pcb = processes + index;
    }
    index++;

    return (optional_pcbptr) { .value = pcb };
}

optional_pcbptr create_new_process(loaded_binary* bin, int stack_size)
{
    // try to get a position in the processes list
    optional_pcbptr slot_or_err = find_available_pcb_slot();

    // if that failed, we cannot creat a new process
    if (has_error(slot_or_err)) {
        dbgln("No more process structs!", 24);
        return slot_or_err;
    }

    // allocate stack for the new process
    optional_voidptr stack_top_or_err = malloc_stack(stack_size); // allocate 4Kib stack

    // if that failed, we also can't create a new process
    if (has_error(stack_top_or_err)) {
        dbgln("Error while allocating stack for process", 40);
        return (optional_pcbptr) { .error = stack_top_or_err.error };
    }

    struct process_control_block* pcb = slot_or_err.value;

    // determine next pid
    int pid = next_process_id++;

    // mark process as ready
    pcb->status = PROC_RDY;
    pcb->pid = pid;
    pcb->pc = bin->entrypoint;
    pcb->binary = bin;
    pcb->parent = NULL;
    pcb->asleep_until = 0;
    pcb->stack_top = stack_top_or_err.value;
    // zero out registers
    memset(0, pcb->regs, pcb->regs + 31);
    // load stack top into stack pointer register
    pcb->regs[REG_SP] = (int) stack_top_or_err.value;
    // load pid into a0 register
    pcb->regs[REG_A0] = pid;

    dbgln("Created new process!", 20);

    return (optional_pcbptr) { .value = pcb };
}

optional_pcbptr create_new_thread(struct process_control_block* parent, void* entrypoint, void* args, int stack_size)
{
    // try to get a position in the processes list
    optional_pcbptr slot_or_err = find_available_pcb_slot();

    // if that failed, we cannot creat a new process
    if (has_error(slot_or_err)) {
        dbgln("No more process structs!", 24);
        return slot_or_err;
    }

    // allocate stack for the new process
    optional_voidptr stack_top_or_err = malloc_stack(stack_size); // allocate 4Kib stack

    // if that failed, we also can't create a new process
    if (has_error(stack_top_or_err)) {
        dbgln("Error while allocating stack for thread", 39);
        return (optional_pcbptr) { .error = stack_top_or_err.error };
    }

    struct process_control_block* pcb = slot_or_err.value;

    // determine next pid
    int pid = next_process_id++;

    // mark process as ready
    pcb->status = PROC_RDY;
    pcb->pid = pid;
    pcb->pc = (int) entrypoint;
    pcb->binary = parent->binary;
    pcb->parent = parent;
    pcb->asleep_until = 0;
    pcb->stack_top = stack_top_or_err.value;
    // zero out registers
    memset(0, pcb->regs, pcb->regs + 31);
    // set return address to global thread finalizer
    pcb->regs[REG_RA] = (int) &thread_finalizer;
    // load stack top into stack pointer register
    pcb->regs[REG_SP] = (int) stack_top_or_err.value;
    // copy global pointer from parent
    pcb->regs[REG_GP] = parent->regs[REG_GP];
    // load args pointer into a0 register
    pcb->regs[REG_A0] = (int) args;

    dbgln("Created new thread!", 19);

    return (optional_pcbptr) { .value = pcb };
}

void kill_child_processes(struct process_control_block* pcb)
{
    for (int i = 0; i < PROCESS_COUNT; i++) {
        struct process_control_block* proc = processes + i;
        // if this is not a child process or already exited
        if (proc->parent != pcb || proc->status == PROC_DEAD)
            continue;

        proc->status = PROC_DEAD;
        proc->exit_code = -9;   // set arbitrary exit code
        kill_child_processes(proc);
    }
}

void destroy_process(struct process_control_block* pcb)
{
    // kill child processes
    kill_child_processes(pcb);
    // free allocated stack
    free_stack(pcb->stack_top);
    // make sure the thread is not rescheduled
    pcb->status = PROC_DEAD;
}
