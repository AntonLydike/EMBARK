#include "stdlib.h"

inline void __attribute__((always_inline)) ecall() {
    asm volatile("ecall");
}

// process control
int spawn(void (*child)(void*), void* args);

void sleep(int length) {
    asm volatile(
        "li     a7, 1\n"
        "ecall\n"
        "mv     a0, %0"
        :: "r"(length)
    );
}

int join(int pid, int timeout) {
    return 0;
}

int kill(int pid) {
    return 0;
}

void __attribute__((noreturn)) exit(int code);

// locks
m_lock mutex_create() {
    m_lock result;
    asm (
        "li     a7, 8\n"
        "ecall\n"
        "mv     %0, a0" :
        "=r"(result)
    );
    return result;
}

int mutex_lock(m_lock lock, int timeout) {
    int result;
    asm (
        "li     a7, 9\n"
        "mv     a0, %1\n"
        "mv     a1, %2\n"
        "ecall\n"
        "mv     %0, a0"
        : "=r"(result)
        : "r"(lock), "r"(timeout)
    );
    return result;
}

void mutex_unlock(m_lock lock) {
    asm (
        "li     a7, 10\n"
        "mv     a0, %0" ::
        "r"(lock)
    );
}

void mutex_destroy(m_lock lock) {
    asm (
        "li     a7, 11\n"
        "mv     a0, %0" ::
        "r"(lock)
    );
}
