#ifndef H_MALLOC
#define H_MALLOC

typedef unsigned int size_t;

typedef struct malloc_info {
    void* allocate_memory_end;
    void* allocate_memory_start;
} malloc_info;

void* malloc(size_t size);
// int free(void* ptr);

void* malloc_stack(size_t size);

void malloc_init(malloc_info* info);

#endif