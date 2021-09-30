#ifndef H_ktypes
#define H_ktypes

// define the nullpointer
#define NULL ((void*) 0)

// types with explicit bit-widths
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef unsigned char byte;

/*
 * Error codes
 */

enum error_code {
    ENOCODE = 1,    // invalid syscall code
    EINVAL  = 2,    // invalid argument value
    ENOMEM  = 3,    // not enough memory
    ENOBUFS = 4,    // no space left in buffer
    ESRCH   = 5,    // no such process
    ETIMEOUT= 6     // timeout while waiting
};

/*
 * Scheduling types
 */

enum process_status {
    PROC_DEAD       = 0,
    PROC_RDY        = 1,
    PROC_WAIT_PROC  = 2,
    PROC_WAIT_SLEEP = 3,
};

// forward define structs for recursive references
struct process_control_block;
struct loaded_binary;

struct process_control_block {
    int pid;
    // state information
    int pc;
    int regs[31];
    int exit_code;
    // scheduling information
    enum process_status status;
    struct process_control_block* waiting_for_process;
    unsigned long long int asleep_until;
    // hierarchical information
    struct loaded_binary* binary;
    struct process_control_block* parent;
    // memory management information
    void* stack_top;
};

enum pcb_struct_registers {
    REG_RA  = 0,
    REG_SP  = 1,
    REG_GP  = 2,
    REG_TP  = 3,
    REG_T0  = 4,
    REG_T1  = 5,
    REG_t2  = 6,
    REG_FP  = 7,
    REG_S0  = 7,
    REG_S1  = 8,
    REG_A0  = 9,
    REG_S2  = 17,
    REG_T3  = 27
};


/* This struct holds information about binaries which are currently loaded into
 * memory. Currently the kernel is not able to load binaries into memory, as
 * no file system layer is implemented. When the memory image is built, the
 * list of loaded binaries is populated aswell.
 */
typedef struct loaded_binary {
    int binid;
    int entrypoint;
    void* bounds[2];
} loaded_binary;


/*
 * Optionals
 *
 * in this kernel, an optional can hold a value or an error, but we can't use
 * unions here because we need to be able to distinguish errors from results.
 * this is a little space inefficient, but we'll have to deal with this, or else
 * we get global errno variables.
 */

#define CreateOptionalOfType(type)          \
    typedef struct Optional ## type {       \
        enum error_code error;              \
        type value;                         \
    } optional_ ## type

// has_value and has_error are not dependent on the value type
// therefore we can define them as macros
#define has_value(optional) (optional.error == 0)
#define has_error(optional) (!has_value(optional))

// define some type aliases that only contain ascii character
// they are used in the name of the struct optional_<name>
typedef void* voidptr;
typedef struct process_control_block* pcbptr;
// size_t is another standard type
typedef unsigned int size_t;

// create optionals for required types
CreateOptionalOfType(int);
CreateOptionalOfType(size_t);
CreateOptionalOfType(pcbptr);
CreateOptionalOfType(voidptr);

/*
 * Stacks
 *
 * stacks allow pushing and popping elements off it.
 */

// we currently only need a voidptr stack, if we need more we can either move them to a separate file
// or just type cast the pointers afterwards
struct voidptr_stack {
    void** data;
    int pos;
    int size;
};

inline void* voidptr_stack_pop(struct voidptr_stack* stack) __attribute__((always_inline));
inline int voidptr_stack_push(struct voidptr_stack* stack, void* ptr) __attribute__((always_inline));
inline void voidptr_stack_new(struct voidptr_stack* stack, void** data, int size) __attribute__((always_inline));

inline void* voidptr_stack_pop(struct voidptr_stack* stack)
{
    if (stack->pos == -1)
        return NULL;
    return stack->data[stack->pos--];
}

inline int voidptr_stack_push(struct voidptr_stack* stack, void* ptr)
{
    if (stack->pos + 1 > stack->size)
        return 0;

    stack->pos += 1;
    stack->data[stack->pos] = ptr;
    return 1;
}

inline void voidptr_stack_new(struct voidptr_stack* stack, void** data, int size)
{
    stack->pos = -1;
    stack->size = size;
    stack->data = data;
}

#endif
