#pragma once
#define TEXT_IO_ADDR 0xff0000
#define TEXT_IO_BUFLEN 64

// these parts are taken from the ktypes.h file
enum error_code {
    ENOCODE = 1,    // invalid syscall code
    EINVAL  = 2,    // invalid argument value
    ENOMEM  = 3,    // not enough memory
    ENOBUFS = 4,    // no space left in buffer
    ESRCH   = 5,    // no such process
    ETIMEOUT= 6     // timeout while waiting
};

struct optional_int {
    enum error_code error;
    int value;
};
// has_value and has_error are not dependent on the value type
// therefore we can define them as macros
#define has_value(optional) (optional.error == 0)
#define has_error(optional) (!has_value(optional))

void dbgln(char*, int);
char* itoa(int value, char* str, int base);

int thread(void* args);

__attribute__((naked)) struct optional_int spawn(int (*target)(void*), void* args) {
    __asm__ (
         "li a7, 1\n"
         "ecall\n"
         "ret"
    );
    __builtin_unreachable();
}

__attribute__((naked)) struct optional_int join(int pid, int timeout) {
    __asm__ (
         "li a7, 3\n"
         "ecall\n"
         "ret"
    );
    __builtin_unreachable();
}


__attribute__((naked)) struct optional_int sleep(int timeout) {
    __asm__ (
         "li a7, 2\n"
         "ecall\n"
         "ret"
    );
    __builtin_unreachable();
}
