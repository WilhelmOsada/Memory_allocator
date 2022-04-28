#ifndef HEAP_FUNC
#define HEAP_FUNC
#include <stdlib.h>
#include <stdio.h>
#include "string.h"
#define FENCE_SIZE 1
#define ALIGNMENT 4096
#define ALIGN(x) (((x) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};
struct __attribute__ ((packed)) memory_chunk_t
{
    struct memory_chunk_t* next;
    struct memory_chunk_t* prev;
    int size;
    int help;
    size_t control;
};
struct memory_manager_t
{
    short init;
    void *memory_start;
    size_t memory_size;
    struct memory_chunk_t *first_memory_chunk;
};
struct memory_manager_t memory_manager;

int heap_validate(void);

int heap_setup(void);
void heap_clean(void);

void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t count);
void  heap_free(void* memblock);

enum pointer_type_t get_pointer_type(const void* const pointer);

size_t   heap_get_largest_used_block_size(void);

void* heap_malloc_aligned(size_t count);
void* heap_calloc_aligned(size_t number, size_t size);
void* heap_realloc_aligned(void* memblock, size_t size);
#endif