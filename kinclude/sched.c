#include "../kernel.h"
#include "sched.h"
#include "mutex.h"
#include "csr.h"


// scheduling data:
ProcessControlBlock processes[PROCESS_COUNT];
int current_process_index = 1;

void scheduler_run_next ()  
{
    current_process_index = scheduler_select_free();
    // set up timer interrupt
    unsigned long long int mtimecmp = read_time() + TIME_SLICE_LEN;
    write_mtimecmp(mtimecmp);
    scheduler_switch_to(current_process_index);
}

int  scheduler_select_free() 
{
    long long int mtime;
    int i;
    int timeout_available = false; // note if a timeout is available

    while (true) {
        mtime = read_time();

        for (i=1; i < PROCESS_COUNT; i++) {
            ProcessControlBlock *pcb = processes + ((current_process_index + i) % PROCESS_COUNT);

            if (pcb->status == PROC_RDY) 
                return (current_process_index + i) % PROCESS_COUNT;
            
            if (pcb->status == PROC_WAIT_SLEEP) {
                if (pcb->asleep_until < mtime) {
                    return (current_process_index + i) % PROCESS_COUNT;
                }
                timeout_available = true;
            }

            if (pcb->status == PROC_WAIT_PROC) {
                if (pcb->asleep_until != 0) {
                    if (pcb->asleep_until < mtime) {
                        // set process return args!
                        return (current_process_index + i) % PROCESS_COUNT;
                    }
                    timeout_available = true;
                }
            }

            if (pcb->status == PROC_WAIT_LOCK) {
                if (pcb->asleep_until != 0) {
                    if (pcb->asleep_until < mtime) {
                        // set process return args!
                        return (current_process_index + i) % PROCESS_COUNT;
                    }
                    timeout_available = true;
                }

                if (!mutex_is_locked(pcb->requested_lock)) {
                    return (current_process_index + i) % PROCESS_COUNT;
                }
            }
        }

        if (timeout_available == false) {
            // either process deadlock or no processes alive. 
            //TODO: handle missing executable thread
            HALT(22);
        }
    }
}

void scheduler_switch_to(int proc_index) 
{
    ProcessControlBlock *pcb = processes + proc_index;

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
    return processes[current_process_index].regs;
}

ProcessControlBlock* get_current_process() 
{
    return &processes[current_process_index];
}
