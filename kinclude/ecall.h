#ifndef H_ECALL
#define H_ECALL

// syscall handlers, are setup in the mtvec csr
void ecall_handle_fork();
void ecall_handle_sleep(int until);
void ecall_handle_wait(int pid, int timeout);
void ecall_handle_kill(int pid);
void ecall_handle_exit(int status);
void ecall_handle_m_create();
void ecall_handle_m_lock(int mutex_id);
void ecall_handle_m_unlock(int mutex_id);
void ecall_handle_m_destroy(int mutex_id);

void __attribute__((__noreturn__)) trap_handle(int interrupt_bit, int code, int mtval);





#endif