#include "ecall.h"
#include "sched.h"
#include "csr.h"

void ecall_handle_spawn(void* entry, void* args)
{

}

void ecall_handle_sleep(int until)
{

}

void ecall_handle_join(int pid, int timeout)
{

}

void ecall_handle_kill(int pid)
{

}

void ecall_handle_exit(int status)
{

}

void trap_handle_ecall() {
    int *regs = get_current_process_registers();
    int code = regs[16];
    __asm__("ebreak");
    HALT(18);
}

void trap_handle(int interrupt_bit, int code, int mtval) 
{
    if (interrupt_bit) {
        switch (code) {
            case 7:
              scheduler_run_next();
              break;
            default:
              // impossible
              HALT(12)
              break;
        }
    } else {
        switch (code) {
            case 8:    // user ecall
                trap_handle_ecall();
                break;
            default:
                HALT(13);
        }
    }
    HALT(1);
    __builtin_unreachable();
}
