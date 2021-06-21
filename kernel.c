#include "kernel.h"
#include "ecall.h"
#include "sched.h"
#include "mutex.h"

void thread_1();

extern ProcessControlBlock processes[PROCESS_COUNT];

extern void init()
{
    // set up processes
    processes[0].pid = 1;
    processes[0].pc = (int) thread_1;
    processes[0].regs[2] = 128;
    processes[0].status = PROC_RDY;
    processes[0].requested_lock = 0;

    processes[1].pid = 2;
    processes[1].pc = (int) thread_1;
    processes[1].regs[2] = 256;
    processes[1].status = PROC_RDY;
    processes[1].requested_lock = 0;

    scheduler_run_next();   
}

void thread_1() {
    int a = 0;  // a4
    int b = 0;  // a5

    while (true) {
        a++;
        if (a > 1000000) {
            __asm__ __volatile__ (
                "ebreak"
            );
            b++;
            a = 0;
        }
        if (b > 1000000) {
            b = 0;
        }
    }
}