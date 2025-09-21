#include <string.h>

void Bun_Arena_Init_From_Allocator(Bun_Arena *arena, Bun_Allocator *allocator, Bun_U32 buffer_size, bool zeroed, Bun_U32 alignment)
{
    arena->buffer = Bun_Allocator_Alloc(buffer_size, zeroed, alignment, allocator);
    arena->buffer_size = buffer_size;
    arena->offset = 0;
}
void Bun_Arena_Deinit_From_Allocator(Bun_Arena *arena, Bun_Allocator *allocator)
{
    Bun_Allocator_Free(arena->buffer, allocator);
    memset( arena, 0, sizeof(arena) );
}
void *Bun_Arena_Alloc(Bun_U32 size, bool zeroed, Bun_U32 alignment, Bun_Arena *arena)
{
    uintptr_t current_pointer, offset;
    void *ptr;

    current_pointer = (uintptr_t)arena->buffer + (uintptr_t)arena->offset;
    current_pointer = (uintptr_t)Bun_Align_Formula(current_pointer, alignment);
    offset = current_pointer - (uintptr_t)arena->buffer;

    if ( offset + size > arena->buffer_size ) return NULL;

    ptr = &arena->buffer[offset];
    arena->offset = offset + size;

    if (zeroed) memset(ptr, 0, size);

    return ptr;
}

void *Bun_Arena_Resize(void *old_memory, Bun_U32 size, Bun_U32 old_size, bool zeroed, Bun_U32 alignment, Bun_Arena *arena)
{
    uintptr_t offset, old_memory_offset;

    if (old_memory == NULL
    || old_size == 0
    || (uintptr_t)old_memory < (uintptr_t)arena->buffer
    || (uintptr_t)old_memory + size >= (uintptr_t)arena->buffer + arena->buffer_size
    ) return NULL;

    if (size < old_size)
    {
        arena->offset -= old_size - size;
        return old_memory;
    }

    old_memory_offset = (uintptr_t)old_memory - (uintptr_t)arena->buffer;
    offset = old_memory_offset + (uintptr_t)old_size;
    if (offset == (uintptr_t)arena->offset)
    {
        offset = old_memory_offset + size;
        if (offset > arena->buffer_size) return NULL;
        arena->offset = offset;
        return old_memory;
    }
    else
    {
        void *new_memory = Bun_Arena_Alloc(size, zeroed, alignment, arena);
        if (new_memory == NULL) return NULL;
        return memmove(new_memory, old_memory, size);
    }
}

void Bun_Arena_Free_All(Bun_Arena *arena)
{
    arena->offset = 0;
}

bool Bun_Dynamic_Arena_Init( Bun_Dynamic_Arena *arena, Bun_Allocator *backing_allocator, Bun_U32 pool_size, bool pool_zeroed, Bun_U32 pool_alignment )
{
    static const Bun_Allocator_Mode required_modes = BUN_ALLOCATOR_MODE_ALLOC
                                               | BUN_ALLOCATOR_MODE_ALLOC_NON_ZEROED
                                               | BUN_ALLOCATOR_MODE_RESIZE
                                               | BUN_ALLOCATOR_MODE_FREE;
    if (!arena || !backing_allocator || !pool_size
    || required_modes &~ backing_allocator->implemented_modes
    ) return false;

    arena->allocator      = backing_allocator;
    arena->pool_size      = pool_size;
    arena->pool_zeroed    = pool_zeroed;
    arena->pool_alignment = pool_alignment;
    arena->pool_offset    = 0;

    arena->pool_len = 8;
    arena->pools = Bun_Allocator_Alloc(sizeof(Bun_Arena)*arena->pool_len, true, BUN_ALLOCATOR_DEFAULT_ALIGN, backing_allocator);
    if (arena->pools == NULL) return false;

    /* this is actually not a good idea, just let the first allocation handle it.
    arena->pools[0].buffer = Allocator_Alloc(pool_size, pool_zeroed, pool_alignment, backing_allocator);
    arena->pools[0].offset = 0;
    */
}
void Bun_Dynamic_Arena_Deinit( Bun_Dynamic_Arena *arena )
{
    if (!arena || !arena->allocator || !arena->pools) return;

    int i;
    for ( i = 0; i < arena->pool_len; i++ )
    {
        if (arena->pools[i].buffer == NULL) break;
        Bun_Allocator_Free(arena->pools[i].buffer, arena->allocator);
    }
    Bun_Allocator_Free(arena->pools, arena->allocator);
}

void *Bun_Dynamic_Arena_Alloc_Push(Bun_U32 size, bool zeroed, Bun_U32 alignment, Bun_Dynamic_Arena *arena)
{
    uintptr_t current_pointer, offset;
    void *ptr;
    Bun_Arena *pool;

    /*Im blanking on if `type x = y[]` is a copy or not, I think it is, but have no internet to check*/
    pool = &arena->pools[arena->pool_offset];

    current_pointer = (uintptr_t)pool->buffer + (uintptr_t)pool->offset;
    offset = (uintptr_t)Bun_Align_Formula(current_pointer, alignment) - (uintptr_t)pool->buffer;

    if ( offset + size > pool->buffer_size )
    {
        Bun_U32 pool_size = (arena->pool_size > size) ? arena->pool_size : size;

        if (pool->buffer != NULL) arena->pool_offset += 1;

        if (arena->pool_offset >= arena->pool_len)
        {
            Bun_U32 old_size = sizeof(Bun_Arena)*arena->pool_len;
            Bun_Arena *new_ptr;
            
            arena->pool_len += 8;
            new_ptr = Bun_Allocator_Resize( arena->pools,
                                        sizeof(Bun_Arena)*arena->pool_len, old_size,
                                        true, BUN_ALLOCATOR_DEFAULT_ALIGN, arena->allocator);
            if (!new_ptr)
                return NULL;
            arena->pools = new_ptr;

        }
        pool = &arena->pools[arena->pool_offset];
        offset = 0;
        Bun_Arena_Init_From_Allocator( pool, arena->allocator, pool_size, arena->pool_zeroed, arena->pool_alignment );
        if (pool->buffer == NULL) return NULL;
    }

    ptr = &pool->buffer[offset];
    pool->offset = offset + size;

    if (zeroed) memset(ptr, 0, size);

    return ptr;
}
void *Bun_Dynamic_Arena_Alloc_Insert(Bun_U32 size, bool zeroed, Bun_U32 alignment, Bun_Dynamic_Arena *arena)
{
    uintptr_t current_pointer, offset;
    void *ptr;
    Bun_Arena *pool;

    if (size > arena->pool_size) return Bun_Dynamic_Arena_Alloc_Push(size, zeroed, alignment, arena);

    int i;
    for (i = 0; i < arena->pool_len; i++)
    {
        pool = &arena->pools[i];

        current_pointer = (uintptr_t)pool->buffer + (uintptr_t)pool->offset;
        offset = (uintptr_t)Bun_Align_Formula(current_pointer, alignment) - (uintptr_t)pool->buffer;

        if ( offset + size > pool->buffer_size ) continue;

        ptr = &pool->buffer[offset];
        pool->offset = offset + size;

        if (zeroed) memset(ptr, 0, size);

        return ptr;
    }
    /* if we reach here there are no gaps to fill */
    return Bun_Dynamic_Arena_Alloc_Push(size, zeroed, alignment, arena);
}
void *Bun_Dynamic_Arena_Resize(void *old_memory, Bun_U32 size, Bun_U32 old_size, bool zeroed, Bun_U32 alignment, Bun_Dynamic_Arena *arena)
{
    uintptr_t current_pointer, offset;
    void *ptr;
    Bun_Arena *pool;
    int i;

    for (i = 0; i < arena->pool_offset; i++)
    {
        pool = &arena->pools[i];
        if ((uintptr_t)old_memory < (uintptr_t)pool->buffer || (uintptr_t)old_memory >= (uintptr_t)pool->buffer + pool->buffer_size)
            continue;
        /* pool found */
        offset = (Bun_Byte*)old_memory - pool->buffer;
        if (offset == pool->offset - old_size) /*Is on the end*/
        {
            if (offset + size >= pool->buffer_size)
            {
                pool->offset = offset;
                ptr = Bun_Dynamic_Arena_Alloc_Push(size, zeroed, alignment, arena);
                if (ptr == NULL) return NULL;
                return memmove(ptr, old_memory, size);
            }
            else if (size < old_size)
            {
                pool->offset = offset + size;
                return old_memory;
            }
            else
            {
                pool->offset = offset + size;
                return old_memory;
            }
        }
        else if (size <= old_size)  /*shrink in the middle of allocated mem (do nothing)*/
        {
            return old_memory;
        }
        else /*shrink in the middle of allocated mem (move) */
        {
            ptr = Bun_Dynamic_Arena_Alloc_Push(size, zeroed, alignment, arena);
            if (ptr == NULL) return NULL;
            return memmove(ptr, old_memory, size);
        }

    }
    /* If here is reached old_memory is not a valid pointer */
    return NULL;
}
void Bun_Dynamic_Arena_Free_All(Bun_Dynamic_Arena *arena, bool zero_pools)
{
    if (arena == NULL) return;
    int i;
    for (i = 0; i < arena->pool_len; i++)
    {
        Bun_Arena *pool = &arena->pools[i];
        pool->offset = 0;
        if (zero_pools) memset(pool->buffer, 0, pool->buffer_size);
    }
    arena->pool_offset = 0;
}
void Bun_Dynamic_Arena_Free_Pools(Bun_Dynamic_Arena *arena, Bun_U32 min_pools, bool zero_pools)
{
    if (arena == NULL) return;

    if (arena->pool_len < min_pools) Bun_Dynamic_Arena_Free_All(arena, zero_pools);

    int i;
    for (i = 0; i < arena->pool_len; i++)
    {
        Bun_Arena *pool = &arena->pools[i];
        pool->offset = 0;
        if (i >= min_pools) Bun_Allocator_Free(pool->buffer, arena->allocator);
        else if (zero_pools) memset(pool->buffer, 0, pool->buffer_size);
    }
    arena->pools = Bun_Allocator_Resize( arena->pools,
                                   sizeof(Bun_Arena)*min_pools, sizeof(Bun_Arena)*arena->pool_len,
                                   false, BUN_ALLOCATOR_DEFAULT_ALIGN, arena->allocator);
    arena->pool_len = min_pools;
    arena->pool_offset = 0;
}
