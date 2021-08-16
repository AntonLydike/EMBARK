#ifndef H_ECALL
#define H_ECALL

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

// syscall handlers, are setup in the mtvec csr
void ecall_handle_spawn(void* entry, void* args);
void ecall_handle_sleep(int until);
void ecall_handle_join(int pid, int timeout);
void ecall_handle_kill(int pid);
void ecall_handle_exit(int status);


void __attribute__((__noreturn__)) trap_handle(int interrupt_bit, int code, int mtval);

#endif