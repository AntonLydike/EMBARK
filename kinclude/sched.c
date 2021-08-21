#include "../kernel.h"
#include "sched.h"
#include "csr.h"
#include "io.h"
#include "malloc.h"


// scheduling data:
ProcessControlBlock processes[PROCESS_COUNT];
ProcessControlBlock* current_process;
unsigned long long int scheduling_interrupted_start;
unsigned long long int next_interrupt_scheduled_for;
int next_process_id = 1;

void scheduler_run_next ()
{
    current_process = scheduler_select_free();
    char msg[30] = "scheduling ";
    char* end = itoa(current_process->pid, &msg[11], 10);
    dbgln(msg, ((int) end) - ((int) msg));

    // set up timer interrupt
    set_next_interrupt();
    scheduler_switch_to(current_process);
}

void scheduler_try_return_to(ProcessControlBlock* pcb)
{
    if (pcb->status != PROC_RDY) {
        scheduler_run_next();
    } else {
        current_process = pcb;
        // add time spent in ecall handler to the processes time slice
        next_interrupt_scheduled_for = next_interrupt_scheduled_for + (read_time() - scheduling_interrupted_start);
        write_mtimecmp(next_interrupt_scheduled_for);
        scheduler_switch_to(current_process);
    }
}

ProcessControlBlock* scheduler_select_free()
{
    unsigned long long int mtime;
    int i;
    int timeout_available = false; // note if a timeout is available

    while (true) {
        mtime = read_time();

        for (i=0; i < PROCESS_COUNT; i++) {
            ProcessControlBlock* pcb = processes + i;
            if (pcb->status == PROC_RDY && pcb != current_process)
                return pcb;
            
            if (pcb->status == PROC_WAIT_SLEEP) {
                if (pcb->asleep_until < mtime) {
                    //TODO: set wakeup args!
                    return pcb;
                }
                timeout_available = true;
            }

            if (pcb->status == PROC_WAIT_PROC) {
                if (pcb->asleep_until != 0) {
                    if (pcb->asleep_until < mtime) {
                        //TODO: set process return args!
                        return pcb;
                    }
                    timeout_available = true;
                }
            }
        }
        if (current_process->status == PROC_RDY) {
            return current_process;
        }

        if (timeout_available == false) {
            // either process deadlock or no processes alive. 
            //TODO: handle missing executable thread
            dbgln("No thread active!", 17);
            HALT(22);
        }
    }
}

void scheduler_switch_to(ProcessControlBlock* pcb)
{
    CSR_WRITE(CSR_MEPC, pcb->pc);

    // set up registers
    __asm__(
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

int  scheduler_index_from_pid(int pid)
{
    for (int i = 0; i < PROCESS_COUNT; i++) {
        if (processes[i].pid == pid)
            return i;
    }
    return -1;
}

int* get_current_process_registers()
{
    return current_process->regs;
}

ProcessControlBlock* get_current_process()
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

optional_pcbptr find_available_pcb_slot() {
    static int index = 0;
    int start_index = index;
    ProcessControlBlock* pcb = processes + index;

    while (pcb->status != PROC_DEAD) {        
        index = (index + 1) % PROCESS_COUNT;
        if (index == start_index)
            return (optional_pcbptr) { .error = ENOBUFS };
        pcb = processes + index;
    }
    return (optional_pcbptr) { .value = pcb };
}

int create_new_process(loaded_binary* bin, int stack_size)
{
    // try to get a position in the processes list
    optional_pcbptr slot_or_err = find_available_pcb_slot();
    // if that failed, we cannot creat a new process
    if (has_error(slot_or_err)) {
        dbgln("No more process structs!", 24);
        return slot_or_err.error;
    }

    // allocate stack for the new process
    optional_voidptr stack_top_or_err = malloc_stack(stack_size); // allocate 4Kib stack
    // if that failed, we also can't create a new process
    if (has_error(stack_top_or_err)) {
        dbgln("Error while allocating stack for process", 40);
        return stack_top_or_err.error;
    }

    ProcessControlBlock* pcb = slot_or_err.value;

    // determine next pid
    int pid = next_process_id++;

    // mark process as ready
    pcb->status = PROC_RDY;
    pcb->pid = pid;
    pcb->pc = bin->entrypoint;
    pcb->binary = bin;
    // load stack top into stack pointer register
    pcb->regs[1] = (int) stack_top_or_err.value;
    // load pid into a0 register
    pcb->regs[9] = pid;

    dbgln("Created new process!", 20);

}
