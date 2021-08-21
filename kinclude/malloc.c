#include "malloc.h"
#include "ecall.h"

malloc_info global_malloc_info = { 0 };
void* allocate_memory_end;


void malloc_init(malloc_info* given_info)
{
    global_malloc_info = *given_info;
    allocate_memory_end = given_info->allocate_memory_end;
}

optional_voidptr malloc(size_t size)
{
    return (optional_voidptr) { .error = ENOMEM };
}

int free(void* ptr)
{
    return EINVAL;
}

// allocate stack and return a pointer to the *end* of the allocated region
optional_voidptr malloc_stack(size_t size)
{
    void* new_alloc_end = (void*) (((int) allocate_memory_end) - size);
    if (new_alloc_end < global_malloc_info.allocate_memory_start) 
        return (optional_voidptr) { .error = ENOMEM };
    void* stack_top = allocate_memory_end;
    allocate_memory_end = new_alloc_end;
    return (optional_voidptr) { .value = stack_top };
}
