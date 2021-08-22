#ifndef H_ECALL
#define H_ECALL

#include "sched.h"

/*
 * Maps ecall names to numbers
 */
enum ecall_codes {
    ECALL_SPAWN = 1,
    ECALL_SLEEP = 2,
    ECALL_JOIN  = 3,
    ECALL_KILL  = 4,
    ECALL_EXIT  = 5,
};

#define ECALL_TABLE_LEN 8

// initializer for ecall lookup table
void init_ecall_table();

// exception handler
void handle_exception(int ecode, int mtval);

// called by the assembly trap handler in boot.S
void __attribute__((__noreturn__)) trap_handle(int interrupt_bit, int code, int mtval);

#endif