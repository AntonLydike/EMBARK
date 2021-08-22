#include "ktypes.h"
#include "ecall.h"
#include "sched.h"
#include "csr.h"
#include "io.h"

// this type is only used here, therefore we don't need it in the ktypes header
typedef optional_int (*ecall_handler)(int*,ProcessControlBlock*);

ecall_handler ecall_table[ECALL_TABLE_LEN] = { 0 };


// ignore unused parameter errors only for these functions
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

optional_int ecall_handle_spawn_thread(int* args_ptr, ProcessControlBlock* pcb)
{
    void* entry = (void*) args_ptr[0];  // a0
    void* args  = (void*) args_ptr[1];  // a1
    //int   flags = args_ptr[2];          // a2

    optional_pcbptr pcb_or_err = create_new_thread(pcb, entry, args, 1<<14);

    if (has_error(pcb_or_err))
        return (optional_int) { .error = pcb_or_err.error };
    
    return (optional_int) { .value = pcb_or_err.value->pid };
}

optional_int ecall_handle_sleep(int* args, ProcessControlBlock* pcb)
{
    int len = args[0];
    if (len < 0) {
        return (optional_int) { .error = EINVAL };
    }

    if (len > 0) {
        pcb->asleep_until = read_time() + len;
        pcb->status = PROC_WAIT_SLEEP;
    }

    return (optional_int) { .value = 0 };
}

optional_int ecall_handle_join(int* args, ProcessControlBlock* pcb)
{
    int pid = args[0];  // a0

    ProcessControlBlock* target = process_from_pid(pid);

    if (target == NULL)
        return (optional_int) { .error = ESRCH };
    
    if (target->status == PROC_DEAD)
        return (optional_int) { .value = target->exit_code };
    
    pcb->status = PROC_WAIT_PROC;
    pcb->waiting_for_process = target;

    int timeout = args[1];
    if (timeout <= 0)
        return (optional_int) { .value = 0 };

    unsigned int len = (unsigned int) timeout;
    pcb->asleep_until = read_time() + len;
    
    return (optional_int) { .value = 0 };
}

optional_int ecall_handle_kill(int* args, ProcessControlBlock* pcb)
{
    return (optional_int) { .error = EINVAL };
}

optional_int ecall_handle_exit(int* args, ProcessControlBlock* pcb)
{
    pcb->status = PROC_DEAD;
    pcb->exit_code = *args;

    dbgln("exit", 4);

    char msg[34] = "process    exited with code   ";

    itoa(pcb->pid, &msg[8], 10);
    itoa(*args % 10, &msg[28], 10);

    dbgln(msg, 34);

    // recursively kill all child processes
    kill_child_processes(pcb);

    return (optional_int) { .value = 0 };
}

#pragma GCC diagnostic pop

void trap_handle_ecall() { 
    mark_ecall_entry();
    ProcessControlBlock* pcb = get_current_process();
    int *regs = pcb->regs;
    int code = regs[REG_A0 + 7];    // code is inside a7

    dbgln("ecall:", 6);

    // check if the code is too large/small or if the handler is zero
    if (code < 0 || code > ECALL_TABLE_LEN || ecall_table[code] == 0) {
        regs[REG_A0] = ENOCODE;
    } else {
        // run the corresponding ecall handler
        optional_int handler_result = ecall_table[code](&regs[REG_A0], pcb);
        // populate registers with return value and error
        regs[REG_A0] = handler_result.value;
        regs[REG_A0 + 1] = handler_result.error;
    }

    // increment pc of this process to move past ecall instruction
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
    HALT(14);
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
    dbgln("exception encountered!", 17);
    __asm__("ebreak");
}
