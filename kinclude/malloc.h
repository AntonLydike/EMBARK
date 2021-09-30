#ifndef H_MALLOC
#define H_MALLOC

#include "ktypes.h"

struct malloc_info {
    void* allocate_memory_end;
    void* allocate_memory_start;
};

optional_voidptr malloc_stack();
void free_stack(void* stack_top);

void malloc_init(struct malloc_info* info);

#endif
