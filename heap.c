#include "heap.h"
void merge_blocks_next(struct memory_chunk_t *p)
{
    p->size += p->next->size;
    if(p->next->next)
    {
        p->next->next->prev = p;
        p->next = p->next->next;
        p->next->control = (size_t)(p->next->next) + (size_t)(p->next->prev) + (size_t)(abs(p->next->size));
    }
    else{
        p->next = NULL;
    }
}
void merge_blocks_prev(struct memory_chunk_t *p)
{
    p->prev->size += p->size;
    if(p->next)
    {
        p->prev->next = p->next;
        p->next->prev = p->prev;
        p->next->control = (size_t)(p->next->next) + (size_t)(p->next->prev) + (size_t)(abs(p->next->size));
        p->prev->control = (size_t)(p->prev->next) + (size_t)(p->prev->prev) + (size_t)(abs(p->prev->size));
    }
    else p->prev->next = NULL;
}
void check_if_null()
{
    struct memory_chunk_t *p = memory_manager.first_memory_chunk;
    int found = 0;
    while (p != NULL)
    {
        if(p->size > 0)
        {
            found = 1;
            break;
        }
        p = p->next;
    }
    if(found == 0)  memory_manager.first_memory_chunk = NULL;
}
int heap_validate(void)
{
    if(memory_manager.init != 1)    return 2;
    struct memory_chunk_t *p = memory_manager.first_memory_chunk;
    while(p != NULL)
    {
        if(p->size > 0)
        {
            if(p->help != 0)    return 3;
            if(p->control != (size_t)(p->next) + (size_t)(p->prev) + (size_t)abs((p->size)))    return 3;
            for (int i = 0; i < FENCE_SIZE; i++)
            {
                if (*((char *) p + sizeof(struct memory_chunk_t) + FENCE_SIZE + p->size + i) != '#' ||
                    *((char *) p + sizeof(struct memory_chunk_t) + i) != '#')   return 1;
            }
        }
        p = p->next;
    }
    return 0;
}
int heap_setup()
{
    void *start = custom_sbrk(0);
    if(start == (void *) -1)  return -1;
    memory_manager.memory_start = start;
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_size = 0;
    memory_manager.init = 1;
    return 0;
}
void heap_clean(void)
{
    custom_sbrk(-(intptr_t)memory_manager.memory_size);
    memory_manager.memory_size = 0;
    memory_manager.first_memory_chunk = NULL;
    memory_manager.init = 0;
}

void* heap_malloc(size_t size)
{
    if(memory_manager.init != 1)    return NULL;
    if(size < 1)    return NULL;
    if(size > 65057756) return NULL;
    if(!memory_manager.first_memory_chunk)
    {
        memory_manager.first_memory_chunk = custom_sbrk((intptr_t )(size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t)));
        if(memory_manager.first_memory_chunk == (void *) - 1)
        {
            memory_manager.first_memory_chunk = NULL;
            return NULL;
        }
        memory_manager.first_memory_chunk->prev = NULL;
        memory_manager.first_memory_chunk->next = NULL;
        memory_manager.first_memory_chunk->size = (int)size;
        memory_manager.first_memory_chunk->help = 0;
        memory_manager.memory_size += size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t);
        for(int i = 0 ; i < FENCE_SIZE ; i++)
        {
            *((char *)memory_manager.first_memory_chunk + memory_manager.first_memory_chunk->size + i + sizeof(struct memory_chunk_t) + FENCE_SIZE) = '#';
            *((char *)memory_manager.first_memory_chunk + i + sizeof(struct memory_chunk_t)) = '#';
        }
        memory_manager.first_memory_chunk->control = (size_t)(memory_manager.first_memory_chunk->next) +
                (size_t)(memory_manager.first_memory_chunk->prev) + (size_t)(abs(memory_manager.first_memory_chunk->size));
        return (void*)((intptr_t)memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t) + FENCE_SIZE);
    }
    if(heap_validate()) return NULL;
    struct memory_chunk_t *p = memory_manager.first_memory_chunk;
    while(1)
    {
        if(p->size < 0 && (unsigned long)abs(p->size) >= size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t))
        {
            p->size = (int)size;
            for(int i = 0 ; i < FENCE_SIZE ; i++)
            {
                *((char *)p + p->size + i + sizeof(struct memory_chunk_t) + FENCE_SIZE) = '#';
                *((char *)p + i + sizeof(struct memory_chunk_t)) = '#';
            }
            p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
            return (void*)((intptr_t)p + sizeof(struct memory_chunk_t) + FENCE_SIZE);
        }
        if(!p->next)    break;
        p = p->next;
    }
    p->next = (struct memory_chunk_t *)(custom_sbrk((intptr_t )(size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t))));
    if(p->next == (void *) - 1)
    {
        p->next = NULL;
        return NULL;
    }
    p->next->prev = p;
    p->next->next = NULL;
    p->next->size = (int)size;
    p->next->help = 0;
    memory_manager.memory_size += size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t);
    for(int i = 0 ; i < FENCE_SIZE ; i++)
    {
        *((char *)p->next + p->next->size + i + sizeof(struct memory_chunk_t) + FENCE_SIZE) = '#';
        *((char *)p->next + i + sizeof(struct memory_chunk_t)) = '#';
    }
    p->next->control = (size_t)(p->next->next) + (size_t)(p->next->prev) + (size_t)(abs(p->next->size));
    p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
    return (void*)((intptr_t)p->next + sizeof(struct memory_chunk_t) + FENCE_SIZE);
}

void* heap_calloc(size_t number, size_t size1)
{
    if(number < 1 || size1 < 1) return NULL;
    size_t size = size1 * number;
    if(size < 1)    return NULL;
    if(size > 65057756) return NULL;
    void *ptr = heap_malloc(size);
    if(!ptr)   return NULL;
    for(size_t i = 0 ; i < size ; i++)  *((char*)ptr + i) = 0;
    return ptr;
}
void* heap_realloc(void* memblock, size_t count)
{
    if(count > 65057756)    return NULL;
    if(heap_validate())    return NULL;
    if(!memblock)   return heap_malloc(count);
    if(get_pointer_type(memblock) != pointer_valid) return NULL;
    if(count == 0)
    {
        heap_free(memblock);
        return NULL;
    }
    struct memory_chunk_t *p = (struct memory_chunk_t *)((intptr_t)memblock - FENCE_SIZE - sizeof(struct memory_chunk_t));
    if(count == (size_t)(p->size))    return memblock;
    if(count > (size_t)(p->size))
    {
        if(p->next)
        {
            if((intptr_t)p->next - (intptr_t)p - (intptr_t)sizeof(struct memory_chunk_t) - 2 * FENCE_SIZE >= (intptr_t)count)
            {
                p->size = (int)count;
                for(int i = 0 ; i < FENCE_SIZE ; i++)
                {
                    *((char*)p + p->size + FENCE_SIZE + sizeof(struct memory_chunk_t) + i) = '#';
                    *((char*)p + i + sizeof(struct memory_chunk_t)) = '#';
                }
                p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
                return memblock;
            }
            else if(p->next->size < 0 && (intptr_t)p->next - (intptr_t)p + ((-1) * p->next->size) - sizeof(struct memory_chunk_t) - 2 * FENCE_SIZE >= count)
            {
                p->size = (int)(((intptr_t)p->next - (intptr_t)p) - sizeof(struct memory_chunk_t) - 2 * FENCE_SIZE);
                p->next->size *= -1;
                merge_blocks_next(p);
                p->size = (int)count;
                for(int i = 0 ; i < FENCE_SIZE ; i++)   *((char*)p + p->size + FENCE_SIZE + i + sizeof(struct memory_chunk_t)) = '#';
                p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
                return memblock;
            }
            else
            {
                void *new = heap_malloc(count);
                if(!new)    return NULL;
                for(int k = 0 ; k < p->size ; k++)
                {
                    *((char *)new + k) = *(char*)((intptr_t)p + (intptr_t)sizeof(struct memory_chunk_t) + FENCE_SIZE + k);
                }
                heap_free(memblock);
                return new;
            }
        }
        else if((intptr_t) custom_sbrk(0) - (intptr_t)p >= (intptr_t)count)
        {
            p->size = (int)count;
            for(int i = 0 ; i < FENCE_SIZE ; i++)   *((char*)p + p->size + FENCE_SIZE + sizeof(struct memory_chunk_t)) = '#';
            p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
            return memblock;
        }
        else
        {
            if(custom_sbrk((intptr_t)( count)) == (void *) -1) return NULL;
            memory_manager.memory_size += count;
            p->size = (int)count;
            for(int i = 0 ; i < FENCE_SIZE ; i++)   *((char*)p + p->size + FENCE_SIZE + sizeof(struct memory_chunk_t) + i) = '#';
            p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
            return memblock;
        }
    }
    else
    {
        p->size = (int)count;
        for(int i = 0 ; i < FENCE_SIZE ; i++)   *((char*)p + p->size + FENCE_SIZE + sizeof(struct memory_chunk_t) + i) = '#';
        p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
        return memblock;
    }
}

void heap_free(void* address)
{
    enum pointer_type_t c = get_pointer_type(address);
    if(c != pointer_valid)    return;
    if(heap_validate()) return;
    struct memory_chunk_t *p = (struct memory_chunk_t *)((intptr_t)address - FENCE_SIZE - sizeof(struct memory_chunk_t));
    if(p->size < 0)    return;
    if(p->next) p->size = (int)((intptr_t)p->next - (intptr_t)p);
    else    p->size = (int)((intptr_t) custom_sbrk(0) - (intptr_t)p);
    p->size = p->size * (-1);
    if(p->next && p->next->size < 0) merge_blocks_next(p);
    if(p->prev && p->prev->size < 0) merge_blocks_prev(p);
    check_if_null();
}
enum pointer_type_t get_pointer_type(const void* const pointer)
{
    if(pointer == NULL) return pointer_null;
    if((intptr_t)pointer - (intptr_t)memory_manager.first_memory_chunk > (intptr_t)memory_manager.memory_size || (intptr_t)pointer < (intptr_t)memory_manager.first_memory_chunk)    return pointer_unallocated;
    if(heap_validate() == 1) return pointer_heap_corrupted;
    struct memory_chunk_t *p = memory_manager.first_memory_chunk;
    if(p == NULL)   return pointer_unallocated;
    while(p->next && (intptr_t)p->next <= (intptr_t)pointer)    p = p->next;
    if(p->size > 0)
    {
        if((intptr_t)pointer < (intptr_t)p + (intptr_t)sizeof(struct memory_chunk_t))    return pointer_control_block;
        if((intptr_t)pointer >= (intptr_t)p + (intptr_t)sizeof(struct memory_chunk_t) && (intptr_t)pointer < (intptr_t)p + (intptr_t)sizeof(struct memory_chunk_t) + FENCE_SIZE)    return pointer_inside_fences;
        if((intptr_t)pointer >= (intptr_t)p + (intptr_t)sizeof(struct memory_chunk_t) + FENCE_SIZE + (intptr_t)p->size && (intptr_t)pointer < (intptr_t)p + (intptr_t)sizeof(struct memory_chunk_t) + 2 * FENCE_SIZE + (intptr_t)p->size)    return pointer_inside_fences;
        if((intptr_t)pointer == (intptr_t)p + (intptr_t)(sizeof(struct memory_chunk_t) + FENCE_SIZE))    return pointer_valid;
        if((intptr_t)pointer > (intptr_t)p + (intptr_t)sizeof(struct memory_chunk_t) + FENCE_SIZE && (intptr_t)pointer < (intptr_t)p + (intptr_t)sizeof(struct memory_chunk_t) + FENCE_SIZE + (intptr_t)p->size)   return pointer_inside_data_block;
        if((intptr_t)pointer >= (intptr_t)p + (intptr_t)sizeof(struct memory_chunk_t) + FENCE_SIZE + (intptr_t)p->size) return pointer_unallocated;
    }
    else
    {
        return pointer_unallocated;
    }
    return pointer_null;
}

size_t   heap_get_largest_used_block_size(void)
{
    if(heap_validate()) return 0;
    struct memory_chunk_t *p = memory_manager.first_memory_chunk;
    int max = 0;
    while(p)
    {
        if(max <  p->size && p->size > 0) max = p->size;
        p = p->next;
    }
    return (size_t)max;
}
struct memory_chunk_t *nextfree_aligned(intptr_t size)
{
    int wrong = ALIGN(memory_manager.memory_size) - memory_manager.memory_size;
    if(custom_sbrk(wrong + ALIGN(size)) == (void *) -1) return NULL;
    memory_manager.memory_size +=  (wrong + ALIGN(size));
    return (struct memory_chunk_t *)((intptr_t)custom_sbrk(0) - (intptr_t)sizeof(struct memory_chunk_t) - (intptr_t)FENCE_SIZE -
            (intptr_t)ALIGN(size));
}
struct memory_chunk_t *nextfree_aligned_r(intptr_t size)
{
    int wrong = ALIGN(memory_manager.memory_size) - memory_manager.memory_size;
    if(custom_sbrk(wrong + ALIGN(size) + PAGE_SIZE) == (void *) -1) return NULL;
    memory_manager.memory_size +=  (wrong + ALIGN(size) + PAGE_SIZE);
    return (struct memory_chunk_t *)((intptr_t)custom_sbrk(0) - (intptr_t)sizeof(struct memory_chunk_t) - (intptr_t)FENCE_SIZE -
                                     (intptr_t)ALIGN(size));
}
void* heap_malloc_aligned(size_t size)
{
    if(memory_manager.init != 1)    return NULL;
    if(size < 1)    return NULL;
    if(size > 65057756) return NULL;
    if(!memory_manager.first_memory_chunk)
    {
        custom_sbrk(ALIGNMENT);
        memory_manager.memory_size += ALIGNMENT;
        intptr_t aligned = ALIGN((intptr_t )(size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t)));
        memory_manager.first_memory_chunk = (struct memory_chunk_t *)((intptr_t)custom_sbrk(aligned) - (intptr_t)sizeof(struct memory_chunk_t) - (intptr_t)FENCE_SIZE);
        if(memory_manager.first_memory_chunk == (void *) - 1)
        {
            memory_manager.first_memory_chunk = NULL;
            return NULL;
        }
        memory_manager.first_memory_chunk->prev = NULL;
        memory_manager.first_memory_chunk->next = NULL;
        memory_manager.first_memory_chunk->size = (int)size;
        memory_manager.first_memory_chunk->help = 0;
        memory_manager.memory_size += aligned;
        for(int i = 0 ; i < FENCE_SIZE ; i++)
        {
            *((char *)memory_manager.first_memory_chunk + memory_manager.first_memory_chunk->size + i + sizeof(struct memory_chunk_t) + FENCE_SIZE) = '#';
            *((char *)memory_manager.first_memory_chunk + i + sizeof(struct memory_chunk_t)) = '#';
        }
        memory_manager.first_memory_chunk->control = (size_t)(memory_manager.first_memory_chunk->next) +
                                                     (size_t)(memory_manager.first_memory_chunk->prev) + (size_t)(abs(memory_manager.first_memory_chunk->size));
        return (void*)((intptr_t)memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t) + FENCE_SIZE);
    }
    if(heap_validate()) return NULL;
    struct memory_chunk_t *p = memory_manager.first_memory_chunk;
    while(1)
    {
        if(p->size < 0 && p->size * (-1) >= (int)(size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t)) && (((intptr_t)p + sizeof(struct memory_chunk_t) + FENCE_SIZE) & (intptr_t)(PAGE_SIZE - 1)) == 0)
        {
            p->size = (int)size;
            for(int i = 0 ; i < FENCE_SIZE ; i++)
            {
                *((char *)p + p->size + i + sizeof(struct memory_chunk_t) + FENCE_SIZE) = '#';
                *((char *)p + i + sizeof(struct memory_chunk_t)) = '#';
            }
            p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
            return (void*)((intptr_t)p + sizeof(struct memory_chunk_t) + FENCE_SIZE);
        }
        if(!p->next)    break;
        p = p->next;
    }
    p->next = nextfree_aligned((intptr_t)size + 2*FENCE_SIZE + (intptr_t)sizeof(struct memory_chunk_t));
    if(p->next == NULL)    return NULL;
    p->next->prev = p;
    p->next->next = NULL;
    p->next->size = (int)size;
    p->next->help = 0;
    for(int i = 0 ; i < FENCE_SIZE ; i++)
    {
        *((char *)p->next + p->next->size + i + sizeof(struct memory_chunk_t) + FENCE_SIZE) = '#';
        *((char *)p->next + i + sizeof(struct memory_chunk_t)) = '#';
    }
    p->next->control = (size_t)(p->next->next) + (size_t)(p->next->prev) + (size_t)(abs(p->next->size));
    p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
    return (void*)((intptr_t)p->next + sizeof(struct memory_chunk_t) + FENCE_SIZE);
}
void* heap_malloc_aligned_r(size_t size)
{
    if(memory_manager.init != 1)    return NULL;
    if(size < 1)    return NULL;
    if(size > 65057756) return NULL;
    if(!memory_manager.first_memory_chunk)
    {
        custom_sbrk(ALIGNMENT);
        memory_manager.memory_size += ALIGNMENT;
        intptr_t aligned = ALIGN((intptr_t )(size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t)));
        memory_manager.first_memory_chunk = (struct memory_chunk_t *)((intptr_t)custom_sbrk(aligned) - (intptr_t)sizeof(struct memory_chunk_t) - (intptr_t)FENCE_SIZE);
        if(memory_manager.first_memory_chunk == (void *) - 1)
        {
            memory_manager.first_memory_chunk = NULL;
            return NULL;
        }
        memory_manager.first_memory_chunk->prev = NULL;
        memory_manager.first_memory_chunk->next = NULL;
        memory_manager.first_memory_chunk->size = (int)size;
        memory_manager.first_memory_chunk->help = 0;
        memory_manager.memory_size += aligned;
        for(int i = 0 ; i < FENCE_SIZE ; i++)
        {
            *((char *)memory_manager.first_memory_chunk + memory_manager.first_memory_chunk->size + i + sizeof(struct memory_chunk_t) + FENCE_SIZE) = '#';
            *((char *)memory_manager.first_memory_chunk + i + sizeof(struct memory_chunk_t)) = '#';
        }
        memory_manager.first_memory_chunk->control = (size_t)(memory_manager.first_memory_chunk->next) +
                                                     (size_t)(memory_manager.first_memory_chunk->prev) + (size_t)(abs(memory_manager.first_memory_chunk->size));
        return (void*)((intptr_t)memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t) + FENCE_SIZE);
    }
    if(heap_validate()) return NULL;
    struct memory_chunk_t *p = memory_manager.first_memory_chunk;
    while(1)
    {
        if(p->size < 0 && p->size * (-1) >= (int)(size + 2*FENCE_SIZE + sizeof(struct memory_chunk_t)) && (((intptr_t)p + sizeof(struct memory_chunk_t) + FENCE_SIZE) & (intptr_t)(PAGE_SIZE - 1)) == 0)
        {
            p->size = (int)size;
            for(int i = 0 ; i < FENCE_SIZE ; i++)
            {
                *((char *)p + p->size + i + sizeof(struct memory_chunk_t) + FENCE_SIZE) = '#';
                *((char *)p + i + sizeof(struct memory_chunk_t)) = '#';
            }
            p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
            return (void*)((intptr_t)p + sizeof(struct memory_chunk_t) + FENCE_SIZE);
        }
        if(!p->next)    break;
        p = p->next;
    }
    p->next = nextfree_aligned_r((intptr_t)size + 2*FENCE_SIZE + (intptr_t)sizeof(struct memory_chunk_t));
    if(p->next == NULL)    return NULL;
    p->next->prev = p;
    p->next->next = NULL;
    p->next->size = (int)size;
    p->next->help = 0;
    for(int i = 0 ; i < FENCE_SIZE ; i++)
    {
        *((char *)p->next + p->next->size + i + sizeof(struct memory_chunk_t) + FENCE_SIZE) = '#';
        *((char *)p->next + i + sizeof(struct memory_chunk_t)) = '#';
    }
    p->next->control = (size_t)(p->next->next) + (size_t)(p->next->prev) + (size_t)(abs(p->next->size));
    p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
    return (void*)((intptr_t)p->next + sizeof(struct memory_chunk_t) + FENCE_SIZE);
}
void* heap_calloc_aligned(size_t number, size_t size1)
{
    if(number < 1 || size1 < 1) return NULL;
    size_t size = size1 * number;
    if(size < 1)    return NULL;
    if(size > 65057756) return NULL;
    void *ptr = heap_malloc_aligned(size);
    if(!ptr)   return NULL;
    for(size_t i = 0 ; i < size ; i++)  *((char*)ptr + i) = 0;
    return ptr;
}
void* heap_realloc_aligned(void* memblock, size_t count)
{
    if(count > 65057756)    return NULL;
    if(heap_validate())    return NULL;
    if(!memblock)   return heap_malloc_aligned_r(count);
    if(get_pointer_type(memblock) != pointer_valid) return NULL;
    if(count == 0)
    {
        heap_free(memblock);
        return NULL;
    }
    struct memory_chunk_t *p = (struct memory_chunk_t *)((intptr_t)memblock - FENCE_SIZE - sizeof(struct memory_chunk_t));
    if(count == (size_t)(p->size))    return memblock;
    if(count > (size_t)(p->size))
    {
        if(p->next)
        {
            if((intptr_t)p->next - (intptr_t)p - (intptr_t)sizeof(struct memory_chunk_t) - 2 * FENCE_SIZE >= (intptr_t)count)
            {
                p->size = (int)count;
                for(int i = 0 ; i < FENCE_SIZE ; i++)
                {
                    *((char*)p + p->size + FENCE_SIZE + sizeof(struct memory_chunk_t) + i) = '#';
                    *((char*)p + i + sizeof(struct memory_chunk_t)) = '#';
                }
                p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
                return memblock;
            }
            else if(p->next->size < 0 && (intptr_t)p->next - (intptr_t)p + ((-1) * p->next->size) - sizeof(struct memory_chunk_t) - 2 * FENCE_SIZE >= count)
            {
                p->size = (int)(((intptr_t)p->next - (intptr_t)p) - sizeof(struct memory_chunk_t) - 2 * FENCE_SIZE);
                p->next->size *= -1;
                merge_blocks_next(p);
                p->size = (int)count;
                for(int i = 0 ; i < FENCE_SIZE ; i++)   *((char*)p + p->size + FENCE_SIZE + i + sizeof(struct memory_chunk_t)) = '#';
                p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
                return memblock;
            }
            else
            {
                void *new = heap_malloc_aligned_r(count);
                if(!new)    return NULL;
                for(int k = 0 ; k < p->size ; k++)
                {
                    *((char *)new + k) = *(char*)((intptr_t)p + (intptr_t)sizeof(struct memory_chunk_t) + FENCE_SIZE + k);
                }
                heap_free(memblock);
                return new;
            }
        }
        else if((intptr_t) custom_sbrk(0) - (intptr_t)p >= (intptr_t)count)
        {
            p->size = (int)count;
            for(int i = 0 ; i < FENCE_SIZE ; i++)   *((char*)p + p->size + FENCE_SIZE + sizeof(struct memory_chunk_t)) = '#';
            p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
            return memblock;
        }
        else
        {
            intptr_t aligned;
            if(count <= ALIGN(memory_manager.memory_size) - memory_manager.memory_size)  aligned = ALIGN(memory_manager.memory_size) - memory_manager.memory_size;
            else    aligned = ALIGN((intptr_t )(count + 2*FENCE_SIZE + sizeof(struct memory_chunk_t)) + ALIGN(memory_manager.memory_size) - memory_manager.memory_size);
            if(custom_sbrk(aligned) == (void *) -1) return NULL;
            memory_manager.memory_size += aligned;
            p->size = (int)count;
            for(int i = 0 ; i < FENCE_SIZE ; i++)   *((char*)p + p->size + FENCE_SIZE + sizeof(struct memory_chunk_t) + i) = '#';
            p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
            return memblock;
        }
    }
    else
    {
        p->size = (int)count;
        for(int i = 0 ; i < FENCE_SIZE ; i++)   *((char*)p + p->size + FENCE_SIZE + sizeof(struct memory_chunk_t) + i) = '#';
        p->control = (size_t)(p->next) + (size_t)(p->prev) + (size_t)(abs(p->size));
        return memblock;
    }
}



