#include "ecall.h"
#include "sched.h"
#include "csr.h"

typedef int (*ecall_handler)(int*,ProcessControlBlock*);

ecall_handler ecall_table[ECALL_TABLE_LEN] = { 0 };

int ecall_handle_spawn_thread(int* args_ptr, ProcessControlBlock* pcb)
{
    void* entry = (void*) args_ptr[0];
    void* args  = (void*) args_ptr[1];

    return EINVAL;
// void* entry, void* args
}

int ecall_handle_sleep(int* args, ProcessControlBlock* pcb)
{
    int len = *args;
    if (len < 0) {
        return EINVAL;
    }

    pcb->asleep_until = read_time() + len;
    pcb->status = PROC_WAIT_SLEEP;

    return 0;
}

int ecall_handle_join(int* args, ProcessControlBlock* pcb)
{
    return EINVAL;
}

int ecall_handle_kill(int* args, ProcessControlBlock* pcb)
{
    return EINVAL;

}

int ecall_handle_exit(int* args, ProcessControlBlock* pcb)
{
    pcb->status = PROC_DEAD;
    pcb->exit_code = *args;

    return 0;
}

void trap_handle_ecall() {
    {
        
        mark_ecall_entry();
    };
    ProcessControlBlock* pcb = get_current_process();
    int *regs = pcb->regs;
    int code = regs[16];

    // check if the code is too large/small or if the handler is zero
    if (code < 0 || code > ECALL_TABLE_LEN || ecall_table[code] == 0) {
        regs[9] = ENOCODE;
        __asm__("ebreak");
    } else {
        // get the corresponding ecall handler
        ecall_handler handler = ecall_table[code];
        regs[9] = handler(&regs[9], pcb);
    }

    // increment pc of this process
    pcb->pc += 4;

    // try to reschedule 
    scheduler_try_return_to(pcb);
}

void trap_handle(int interrupt_bit, int code, int mtval) 
{
    if (interrupt_bit) {
        switch (code) {
            // timer interrupts
            case 4:
            case 5:
            case 6:
            case 7:
              scheduler_run_next();
              break;
            default:
              // impossible
              HALT(12);
              break;
        }
    } else {
        switch (code) {
            // any known exception code:
            case 0:
            case 1:
            case 2:
            case 4:
            case 5:
            case 6:
            case 7:
            case 12:
            case 13:
            case 15:
                handle_exception(code, mtval);
                break;
            // user or supervisor ecall
            case 8:    
            case 9:
                trap_handle_ecall();
                break;
            // unknown code
            default:
                HALT(13);
        }
    }
    HALT(1);
    __builtin_unreachable();
}

void init_ecall_table() 
{
    ecall_table[ECALL_SPAWN] = ecall_handle_spawn_thread;
    ecall_table[ECALL_SLEEP] = ecall_handle_sleep;
    ecall_table[ECALL_JOIN]  = ecall_handle_join;
    ecall_table[ECALL_KILL]  = ecall_handle_kill;
    ecall_table[ECALL_EXIT]  = ecall_handle_exit;
}

void handle_exception(int ecode, int mtval) 
{

}
