#include "ktypes.h"
#include "ecall.h"
#include "sched.h"
#include "csr.h"
#include "io.h"

// this type is only used here, therefore we don't need it in the ktypes header
typedef optional_int (*ecall_handler)(int*, struct process_control_block*);

ecall_handler ecall_table[ECALL_TABLE_LEN] = { 0 };


// ignore unused parameter errors only for these functions
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

optional_int ecall_handle_spawn_thread(int* args_ptr, struct process_control_block* pcb)
{
    // args_ptr is a pointer to the pcb->regs field, starting at a0.
    // args_ptr[0] is a0, args_ptr[1] is a1, etc.
    void* entry = (void*) args_ptr[0];  // a0
    void* args = (void*) args_ptr[1];   // a1

    // create a new thread
    optional_pcbptr pcb_or_err = create_new_thread(pcb, entry, args);

    // if an error occured, pass it along to the process
    if (has_error(pcb_or_err))
        return (optional_int) { .error = pcb_or_err.error };

    return (optional_int) { .value = pcb_or_err.value->pid };
}

optional_int ecall_handle_sleep(int* args, struct process_control_block* pcb)
{
    // read len from a0
    int len = args[0];

    // only allow sleeping for positive intervals
    if (len < 0) {
        return (optional_int) { .error = EINVAL };
    }

    // if a positive interval is given, calculate the wakeup time
    if (len > 0) {
        pcb->asleep_until = read_time() + len;
        pcb->status = PROC_WAIT_SLEEP;
    }

    return (optional_int) { .value = 0 };
}

optional_int ecall_handle_join(int* args, struct process_control_block* pcb)
{
    int pid = args[0];  // read pid from processes a0 register

    // find the referenced process
    struct process_control_block* target = process_from_pid(pid);

    if (target == NULL)
        return (optional_int) { .error = ESRCH };

    // if the process is dead, join can return immediately
    if (target->status == PROC_DEAD)
        return (optional_int) { .value = target->exit_code };

    // mark the current process as waiting for the target process
    pcb->status = PROC_WAIT_PROC;
    pcb->waiting_for_process = target;

    // check if a valid timeout was passed in register a1
    int timeout = args[1];

    if (timeout <= 0)
        return (optional_int) { .value = 0 };

    // set the asleep_until field
    unsigned int len = (unsigned int) timeout;

    pcb->asleep_until = read_time() + len;

    // here we can return whatever value we want, as it is overwritten when
    // the process is awoken again
    return (optional_int) { .value = 0 };
}

optional_int ecall_handle_kill(int* args, struct process_control_block* pcb)
{
    int pid = args[0];
    struct process_control_block* target = process_from_pid(pid);

    // return error if no process has that id
    if (target == NULL)
        return (optional_int) { .error = ESRCH };

    // return success if the process is dead
    if (target == PROC_DEAD)
        return (optional_int) { .value = 1 };

    // kill target by marking it dead
    target->status = PROC_DEAD;
    target->exit_code = -10;    // set unique exit code

    // call process destructor
    destroy_process(pcb);

    // return success
    return (optional_int) { .value = 1 };
}

optional_int ecall_handle_exit(int* args, struct process_control_block* pcb)
{
    pcb->status = PROC_DEAD;
    pcb->exit_code = *args;

    // print a message if debugging is enabled
    if (DEBUGGING) {
        char msg[34] = "process    exited with code   ";

        itoa(pcb->pid, &msg[8], 10);
        itoa(*args, &msg[28], 10);

        dbgln(msg, 34);
    }

    // call process destructor
    destroy_process(pcb);

    return (optional_int) { .value = 0 };
}

#pragma GCC diagnostic pop

void trap_handle_ecall()
{
    // save current clock so we don't waste too much process time
    mark_ecall_entry();
    // get the current process
    struct process_control_block* pcb = get_current_process();
    int *regs = pcb->regs;
    int code = regs[REG_A0 + 7];    // syscall code is stored inside a7

    // check if the code is too large/small or if the handler is zero
    if (code < 0 || code > ECALL_TABLE_LEN || ecall_table[code] == NULL) {
        regs[REG_A0] = ENOCODE;
    } else {
        // run the corresponding ecall handler
        optional_int handler_result = ecall_table[code](&regs[REG_A0], pcb);
        // populate registers with return value and error
        regs[REG_A0] = handler_result.error;
        regs[REG_A0 + 1] = handler_result.value;
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
            // any other interrupt is not supported currently
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

// this writes the function pointers to the ecall table
// it's called from the kernels init() function
void init_ecall_table()
{
    ecall_table[ECALL_SPAWN] = ecall_handle_spawn_thread;
    ecall_table[ECALL_SLEEP] = ecall_handle_sleep;
    ecall_table[ECALL_JOIN] = ecall_handle_join;
    ecall_table[ECALL_KILL] = ecall_handle_kill;
    ecall_table[ECALL_EXIT] = ecall_handle_exit;
}

// this exception handler is crude and just kills off any process who
// causes an exception.
void handle_exception(int ecode, int mtval)
{
    // kill off offending process
    struct process_control_block* pcb = get_current_process();

    pcb->status = PROC_DEAD;
    pcb->exit_code = -99;
    destroy_process(pcb);

    // run the next process
    scheduler_run_next();
}
