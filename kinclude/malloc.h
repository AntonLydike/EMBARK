#ifndef H_MALLOC
#define H_MALLOC

#include "ktypes.h"

typedef struct malloc_info {
    void* allocate_memory_end;
    void* allocate_memory_start;
} malloc_info;

optional_voidptr malloc_stack(size_t size);

void malloc_init(malloc_info* info);

#endif
