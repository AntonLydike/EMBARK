#ifndef H_ktypes
#define H_ktypes

/*
 *  Error codes
 */

enum error_code {
    ENOCODE = -1,   // invalid syscall code
    EINVAL  = -2,   // invalid argument value
    ENOMEM  = -3,   // not enough memory
};

/*
 *  Scheduling types
 */

enum process_status {
    PROC_DEAD       = 0,
    PROC_RDY        = 1,
    PROC_WAIT_PROC  = 2,
    PROC_WAIT_SLEEP = 3,
};

// process structure:
typedef struct ProcessControlBlock ProcessControlBlock;
struct ProcessControlBlock {
    int pid;
    int pc;
    int regs[31];
    int exit_code;
    // scheduling information
    enum process_status status;
    ProcessControlBlock *waiting_for_process;
    unsigned long long int asleep_until;
};

/*
 *  Optionals
 *
 * in this kernel, an optional can hold a value or an error, but we can't use 
 * unions here because we need to be able to distinguish errors from results.
 * this is a little space inefficient, but we'll have to deal with this, or else
 * we get global errno variables.
 */

#define CreateOptionalOfType(type)\
typedef struct Optional##type { \
    enum error_code error;      \
    type value;                 \
} optional_##type

#define has_value(optional) (optional.error == 0)
#define has_error(optional) (!has_value(optional))


typedef unsigned int size_t;
typedef void* voidptr;
typedef ProcessControlBlock* pcbptr;

// create optionals for required types
CreateOptionalOfType(int);
CreateOptionalOfType(size_t);
CreateOptionalOfType(pcbptr);
CreateOptionalOfType(voidptr);

#endif
