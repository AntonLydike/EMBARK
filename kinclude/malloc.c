#include "../kernel.h"
#include "ktypes.h"
#include "malloc.h"
#include "ecall.h"
#include "io.h"

// information about the systems memory layout is stored here
static malloc_info global_malloc_info = { 0 };
// this pointer points to the end of unused memory
static void* allocate_memory_end;
// this stack holds currently unused program stacks
// it is a stack in the sense that you can push and pop variables
static struct voidptr_stack unused_stacks;

void stash_stack(void* stack_top)
{
    if (voidptr_stack_push(&unused_stacks, stack_top) == 0) {
        // if this happens, we know we messed up and created more stacks than we
        // could ever have processes running simultaneously!
        // we probably have an erro in retrieving stashed stacks?
        dbgln("cannot stash stack, no space left! This should never ever happen!", 65);
    }
}

void malloc_init(malloc_info* given_info)
{
    global_malloc_info = *given_info;
    allocate_memory_end = given_info->allocate_memory_end;
    allocate_memory_end = (void*) (((int) allocate_memory_end) - PROCESS_COUNT);
    voidptr_stack_new(&unused_stacks, (void**) allocate_memory_end, PROCESS_COUNT);
}

// allocate stack and return a pointer to the *end* of the allocated region
// this doesn't reuse stack from functions which exited
optional_voidptr malloc_stack()
{
    // try to reuse a stack top
    void* stack_top = voidptr_stack_pop(&unused_stacks);

    if (stack_top != NULL) {
        return (optional_voidptr) { .value = stack_top };
    }

    // otherwise allocate a new stack at the end of available memory
    void* new_alloc_end = (void*) (((int) allocate_memory_end) - USER_STACK_SIZE);

    // check if we ran out of space
    if (new_alloc_end < global_malloc_info.allocate_memory_start)
        return (optional_voidptr) { .error = ENOMEM };

    stack_top = allocate_memory_end;
    allocate_memory_end = new_alloc_end;
    return (optional_voidptr) { .value = stack_top };
}

void free_stack(void* stack_top)
{
    stash_stack(stack_top);
}
