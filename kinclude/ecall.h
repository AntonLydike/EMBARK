#ifndef H_ECALL
#define H_ECALL

#include "sched.h"

/* ecall codes are defined here
 *
 *
 */
enum ecall_codes {
    ECALL_SPAWN = 1,
    ECALL_SLEEP = 2,
    ECALL_JOIN  = 3,
    ECALL_KILL  = 4,
    ECALL_EXIT  = 5,
};

#define ECALL_TABLE_LEN 16

enum ecall_errors {
    ENOCODE = -1,   // invalid syscall code
    EINVAL  = -2,   // invalid argument value
    ENOMEM  = -3,   // not enough memory
};

// initializer for ecall lookup table
void init_ecall_table();

// syscall handlers, are setup in the mtvec csr
int ecall_handle_spawn(int*, ProcessControlBlock*);
int ecall_handle_sleep(int*, ProcessControlBlock*);
int ecall_handle_join(int*, ProcessControlBlock*);
int ecall_handle_kill(int*, ProcessControlBlock*);
int ecall_handle_exit(int*, ProcessControlBlock*);


void __attribute__((__noreturn__)) trap_handle(int interrupt_bit, int code, int mtval);

#endif