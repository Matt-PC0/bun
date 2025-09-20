/*
BUN - Base lib for c programming.

Header acts as a single header lib although a static lib is also compileable.

STANDARD:
    C89 - should be compilable no matter what*

Defines:
    BUN_IMPLEMENTATION - Include Implementation code.
    BUN_NO_MACROS      - Exclude all macro #defines.
    BUN_STRIP_PREFIX   - provide 'bun_' unprifixed aliases

Provides:
    U8-64         - unsigned type aliases.
    S8-64         - signed type aliases.
    String        - String with length.
    Allocator     - A generic allocator interface.
    Arena         - A fixed size arena.
    Dynamic_Arena - A dynamically sized arena.

Usage:
    Single header lib:
        #define BUN_IMPLEMENTATION
        #include "bun.h"
        int main(void)
        {
            //...
        }
    cc main.c -I<path-to-bun.h-dir>

    Static Lib:
        #include "bun.h"
        int main(void)
        {
            //...
        }
    cc main.c -I<path-to-bun.h-dir> -L<path-to-bun.a-dir> -lbun

DISCLAIMER:
    This is not thoroughly tested, it may have horrible memory bugs that
    Will ruin your day.

Refrances:
    https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
    https://pkg.odin-lang.org/core/mem/
    https://github.com/EpicGamesExt/raddebugger/tree/master
*/

#ifndef BUN_H
#define BUN_H

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

typedef int8_t   Bun_S8;
typedef int16_t  Bun_S16;
typedef int32_t  Bun_S32;
typedef int64_t  Bun_S64;

typedef uint8_t  Bun_U8;
typedef uint16_t Bun_U16;
typedef uint32_t Bun_U32;
typedef uint64_t Bun_U64;

/*XXX: x86_64 only */
typedef float       Bun_F32;
typedef double      Bun_F64;
typedef long double Bun_F128;

typedef Bun_U8 Bun_Byte;

typedef struct
{
    char *ptr;
    Bun_U32 len;
} Bun_String;

/* I'm using ODIN as refrence and it uses align_of*/
/* of course c < c11 doesnt have this, but the alignment*/
/* of a base type is equal it its size (I think).*/
/* In anycase this is 2*8 = 16 bytes on x86_64 which is what we want.*/
#define BUN_ALLOCATOR_DEFAULT_ALIGN 2 * sizeof(uintptr_t)

typedef Bun_U16 Bun_Allocator_Error;
enum
{
    BUN_ALLOCATOR_ERROR_NONE,
    BUN_ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED,
    BUN_ALLOCATOR_ERROR_OUT_OF_MEMORY,
    BUN_ALLOCATOR_ERROR_INVALID_POINTER,
    BUN_ALLOCATOR_ERROR_INVALID_ARGUMENT,
    BUN_ALLOCATOR_ERROR_UNKNOWN,
    _BUN_ALLOCATOR_ERROR_COUNT, /*If a custom allocator needs to have more errors have them relitive to this*/
};
typedef Bun_U8 Bun_Allocator_Mode;
enum
{
    BUN_ALLOCATOR_MODE_ALLOC             = (1<<0),
    BUN_ALLOCATOR_MODE_ALLOC_NON_ZEROED  = (1<<1),
    BUN_ALLOCATOR_MODE_FREE              = (1<<2),
    BUN_ALLOCATOR_MODE_FREE_ALL          = (1<<3),
    BUN_ALLOCATOR_MODE_RESIZE            = (1<<4),
    BUN_ALLOCATOR_MODE_RESIZE_NON_ZEROED = (1<<5),
};
typedef void *(*Bun_Allocator_Proc)(
                                void *allocator_data,
                                Bun_Allocator_Error *allocator_error,
                                Bun_Allocator_Mode mode,
                                Bun_U32 size,
                                Bun_U32 alignment,
                                void *old_memory,
                                Bun_U32 old_size
                               );
typedef struct
{
    Bun_Allocator_Proc proc;

    Bun_Allocator_Mode implemented_modes;
    void *data;
    Bun_Allocator_Error error;
} Bun_Allocator;

typedef struct
{
    Bun_Byte *buffer;
    Bun_U32 buffer_size;
    Bun_U32 offset;

} Bun_Arena;

typedef struct
{
    Bun_Arena *pools;
    Bun_U32 pool_len;
    Bun_U32 pool_offset;

    Bun_U32  pool_size;
    bool pool_zeroed;
    Bun_U32  pool_alignment;
    Bun_Allocator *allocator;
} Bun_Dynamic_Arena;

void *Bun_Allocator_Alloc(Bun_U32 size, bool zeroed, Bun_U32 alignment, Bun_Allocator *allocator);
bool Bun_Allocator_Free(void *ptr, Bun_Allocator *allocator);
bool Bun_Allocator_Free_all(Bun_Allocator *allocator);
void *Bun_Allocator_Resize(void *ptr, Bun_U32 size, Bun_U32 old_size, bool zeroed, Bun_U32 alignment, Bun_Allocator *allocator);

#ifndef BUN_NO_MACROS
#define Bun_Allocator_NEW( T, A ) ((T*)Bun_Allocator_Alloc( sizeof(T), true, BUN_ALLOCATOR_DEFAULT_ALIGN, A ))
#endif

void Bun_Arena_Init_From_Allocator(Bun_Arena *arena, Bun_Allocator *allocator, Bun_U32 buffer_size, bool zeroed, Bun_U32 alignment);
void Bun_Arena_Deinit_From_Allocator(Bun_Arena *arena, Bun_Allocator *allocator);
void *Bun_Arena_Alloc(Bun_U32 size, bool zeroed, Bun_U32 alignment, Bun_Arena *arena);
void *Bun_Arena_Resize(void *old_memory, Bun_U32 size, Bun_U32 old_size, bool zeroed, Bun_U32 alignment, Bun_Arena *arena);
void  Bun_Arena_Free_All(Bun_Arena *arena);

/*
Initialise dynamic_arena and allocate the first pool.

ARGS:
    arena             - uninitialised arena.
    backing_allocator - allocator used to allocate pools.
    pool_size         - minimum size in bytes of each backing pool.
    pool_zeroed       - wether to initialise all pools to zero on allocating them.
RETURN:
    true on success, false on failure
*/
bool Bun_Dynamic_Arena_Init( Bun_Dynamic_Arena *arena, Bun_Allocator *backing_allocator, Bun_U32 pool_size, bool pool_zeroed, Bun_U32 pool_alignment );
/*
Deinitialise dynamic_arena and free all pools

ARGS:
    arena - initialised arena
*/
void Bun_Dynamic_Arena_Deinit( Bun_Dynamic_Arena *arena);
/*
Allocate using the dynamic arena, ignoring gaps in pools and attempting to
push the allocation on the last pool.

best for when an allocation is likely to be bigger then any gaps in the pools.
NOTE: alloc_insert will call this itself for allocations where sizes > pool size

ARGS:
    size      - size of allocation in bytes
    zeroed    - wether to initialise memory to zero
    alignment - alignment of allocation
    arena     - an initialised dynamic arena
RETURN:
    Pointer to allocated memory or NULL on failure
*/
void *Bun_Dynamic_Arena_Alloc_Push(Bun_U32 size, bool zeroed, Bun_U32 alignment, Bun_Dynamic_Arena *arena);
/*
Allocate using the dynamic arena, searching for gaps in all pools and
inserting the new allocation in any free gap or appending it.

best for smaller allocations that are likely to fit in skipped gaps.
NOTE: this will call alloc_push for allocations where sizes > pool size

ARGS:
    size      - size of allocation in bytes
    zeroed    - wether to initialise memory to zero
    alignment - alignment of allocation
    arena     - an initialised dynamic arena
RETURN:
    Pointer to allocated memory or NULL on failure
*/
void *Bun_Dynamic_Arena_Alloc_Insert(Bun_U32 size, bool zeroed, Bun_U32 alignment, Bun_Dynamic_Arena *arena);
/*
Resize previusly allocated memory in a dynamic arena.
Will attempt to preserve the pointer if it is the last allocation in a pool
with enough free space to grow, otherwise the pointer is moved.

In general try to avoid overusing resizes as it can ""leak"" memory until
free_all call. (not a actual memory leak, it will freed, but its wasted)

ARGS:
    old_memory - pointer to memory previusly allocated on the arena
    size       - size in bytes of new allocation
    old_size   - size in bytes of old allocation
    zeroed     - wether to initialise memory to zero
    alignment  - alignment of allocation
    arena      - the initialised dynamic arena used to allocate *old_memory*
RETURN:
    Pointer to allocated memory or NULL on failure
*/
void *Bun_Dynamic_Arena_Resize(void *old_memory, Bun_U32 size, Bun_U32 old_size, bool zeroed, Bun_U32 alignment, Bun_Dynamic_Arena *arena);
/*
Free every allocation, but hold onto the allocated pools.
New allocations after a free_all will overwrite the old memory in the pools.

ARGS:
    arena      - an initialised dynamic arena
    zero_pools - set the memory in all the pools to zero (if you use ZII, prefer this to individual zeroed allocs)
*/
void Bun_Dynamic_Arena_Free_All(Bun_Dynamic_Arena *arena, bool zero_pools);
/*
Free every allocation, and free all but untill min_pools pools.
New allocations after a free_all will overwrite the old memory in the pools.

ARGS:
    arena      - an initialised dynamic arena
    min_pools  - number of pools to keep allocated. (individual objects allocated within these pools are still freed)
    zero_pools - set the memory in all the pools to zero (if you use ZII, prefer this to individual zeroed allocs)
*/
void Bun_Dynamic_Arena_Free_Pools(Bun_Dynamic_Arena *arena, Bun_U32 min_pools, bool zero_pools);

extern Bun_Allocator bun_allocator_libc;


/*
Align to nearest *alignment* forward
*/
Bun_U32 Bun_Align_formula( Bun_U32 size, Bun_U32 alignment);

/*
Alias cstring as string; does no allocation/copy.

ARGS:
    cstring - NULL terminated string.
    len     - length of cstring or 0 to use strlen(cstring)
RETURN:
    string with .ptr == cstring and .len == *len* or (U32)strlen(cstring)
*/
Bun_String Bun_String_Alias(const char *cstring, Bun_U32 len);
/*
Copy cstring to a new string using the provided allocator.

ARGS:
    cstring   - NULL terminated string.
    len       - length of cstring or 0 to use strlen(cstring)
    allocator - A valid allocator that supports ALLOCATOR_MODE_ALLOC.
RETURN:
    string with .ptr == cstring and .len == *len* or (U32)strlen(cstring)
*/
Bun_String String_Copy(const char *cstring, Bun_U32 len, Bun_Allocator *allocator);
/*
Dublicate string using the provided allocator.

ARGS:
    string    - A valid bun string.
    allocator - A valid allocator that supports ALLOCATOR_MODE_ALLOC
RETURN:
    A copy of string
*/
Bun_String Bun_String_Duplicate(Bun_String string, Bun_Allocator *allocator);
/*
Check wether string.ptr is null terminated respecting string.len.

ARGS:
    string - A bun string.
RETURN:
    true if string.ptr is null terminated otherwise false
*/
bool Bun_String_Is_Null_Terminated(Bun_String string);

#ifndef BUN_NO_MACROS


/*only in c would one key work mean both internal and global*/
#define BUN_INTERNAL      static
#define BUN_GLOBAL        static

#define BUN_BIT_L(N) (1<<N)
#define BUN_BIT_H(N) (1ull<<N)
#define BUN_BIT(N) ( (N < 32) BIT_L(N) : BIT_H(N) )

#define BUN_DEFER_LOOP( begin, end ) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end)) 

#define BUN_MIN(A,B) (((A)<(B))?(A):(B))
#define BUN_MAX(A,B) (((A)>(B))?(A):(B))
#define BUN_CLAMP(A,X,B) (((X)<(A))?(A):((X)>(B))?(B):(X))

#define BUN_INT_FROM_PTR(ptr) ((uintptr_t)(ptr))

#define BUN_MEMBER(T,m)                   (((T*)0)->m)
#define BUN_OFFSET_OF(T,m)                INT_FROM_PTR(&MEMBER(T,m))
#define BUN_MEMBER_FROM_OFFSET(T,ptr,off) (T)((((U8 *)ptr)+(off)))
#define BUN_CAST_FROM_MEMBER(T,m,ptr)     (T*)(((U8*)ptr) - OFFSET_OF(T,m))

#endif /*BUN_NO_MACROS*/

#endif /*BUN_H*/

#ifdef BUN_IMPLEMENTATION

Bun_U32 Bun_Align_formula( Bun_U32 size, Bun_U32 alignment)
{
    Bun_U32 result = size + alignment-1;
    return result - result % alignment;
}

void *Bun_Allocator_Libc_Proc(void *allocator_data,
                          Bun_Allocator_Error *allocator_error,
                          Bun_Allocator_Mode mode,
                          Bun_U32 size,
                          Bun_U32 alignment,
                          void *old_memory,
                          Bun_U32 old_size
                          )
{
    void * ptr;
    switch (mode)
    {
        case BUN_ALLOCATOR_MODE_ALLOC:
        case BUN_ALLOCATOR_MODE_ALLOC_NON_ZEROED:
            if (mode == BUN_ALLOCATOR_MODE_ALLOC) ptr = calloc( Bun_Align_formula(size, alignment), 1 );
            else                              ptr = malloc( Bun_Align_formula(size, alignment) );
            if (ptr == NULL && allocator_error != NULL)
            {
                *allocator_error = (errno == ENOMEM) ? BUN_ALLOCATOR_ERROR_OUT_OF_MEMORY : BUN_ALLOCATOR_ERROR_UNKNOWN;
            }
            return ptr;
        case BUN_ALLOCATOR_MODE_FREE:
            if (old_memory == NULL)
            {
                if (allocator_error != NULL) *allocator_error = BUN_ALLOCATOR_ERROR_INVALID_POINTER;
                return NULL;
            }
            free(old_memory);
            return old_memory;
        case BUN_ALLOCATOR_MODE_FREE_ALL:
            return NULL; /*unimplemented*/
        case BUN_ALLOCATOR_MODE_RESIZE:
            if (old_size == 0)
            {
                if (allocator_error != NULL) *allocator_error = BUN_ALLOCATOR_ERROR_INVALID_ARGUMENT;
                return NULL;
            }
            /* fallthrough */
        case BUN_ALLOCATOR_MODE_RESIZE_NON_ZEROED:
            if (old_memory == NULL || size == 0)
            {
                if (allocator_error != NULL) *allocator_error = BUN_ALLOCATOR_ERROR_INVALID_ARGUMENT;
                return NULL;
            }
            if (old_memory == NULL)
            {
                if (allocator_error != NULL) *allocator_error = BUN_ALLOCATOR_ERROR_INVALID_POINTER;
                return NULL;
            }

            ptr = realloc( old_memory, Bun_Align_formula(size, alignment) );

            if (ptr == NULL)
            {
                if (allocator_error != NULL)
                {
                    *allocator_error = (errno == ENOMEM) ? BUN_ALLOCATOR_ERROR_OUT_OF_MEMORY : BUN_ALLOCATOR_ERROR_UNKNOWN;
                }
                return NULL;
            }

            /* zero */
            if (mode == BUN_ALLOCATOR_MODE_RESIZE && size > old_size)
                memset(ptr + old_size, 0, size - old_size);

            return ptr;
        default:
            if (allocator_error != NULL) *allocator_error = BUN_ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED;
            return NULL;
    }
}

Bun_Allocator bun_allocator_libc = (Bun_Allocator){
    .proc = &Bun_Allocator_Libc_Proc,
    .implemented_modes = BUN_ALLOCATOR_MODE_ALLOC
                       | BUN_ALLOCATOR_MODE_ALLOC_NON_ZEROED
                       | BUN_ALLOCATOR_MODE_FREE
                       | BUN_ALLOCATOR_MODE_RESIZE
                       | BUN_ALLOCATOR_MODE_RESIZE_NON_ZEROED,
    .data = NULL,
    .error = 0,
};

void *Bun_Allocator_Alloc(Bun_U32 size, bool zeroed, Bun_U32 alignment, Bun_Allocator *allocator)
{
    if (!allocator) return NULL;
    Bun_Allocator_Mode mode = (zeroed) ? BUN_ALLOCATOR_MODE_ALLOC : BUN_ALLOCATOR_MODE_ALLOC_NON_ZEROED;

    if (mode &~ allocator->implemented_modes)
    {
        allocator->error = BUN_ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED;
        return NULL;
    }

    return allocator->proc( &allocator->data, &allocator->error,
                            mode,
                            size,
                            alignment,
                            NULL,
                            0
                          );
}
bool Bun_Allocator_Free(void *ptr, Bun_Allocator *allocator)
{
    if (!allocator) return NULL;

    if (BUN_ALLOCATOR_MODE_FREE &~ allocator->implemented_modes)
    {
        allocator->error = BUN_ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED;
        return false;
    }

    return allocator->proc( &allocator->data, &allocator->error,
                            BUN_ALLOCATOR_MODE_FREE,
                            0,
                            0,
                            ptr,
                            0
                          ) != NULL;

}
bool Bun_Allocator_Free_all(Bun_Allocator *allocator)
{
    if (!allocator) return NULL;

    if (BUN_ALLOCATOR_MODE_FREE_ALL &~ allocator->implemented_modes)
    {
        allocator->error = BUN_ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED;
        return NULL;
    }

    return allocator->proc( &allocator->data, &allocator->error,
                            BUN_ALLOCATOR_MODE_FREE_ALL,
                            0,
                            0,
                            NULL,
                            0
                          ) != NULL;
}
void *Bun_Allocator_Resize(void *ptr, Bun_U32 size, Bun_U32 old_size, bool zeroed, Bun_U32 alignment, Bun_Allocator *allocator)
{
    if (!allocator) return NULL;
    Bun_Allocator_Mode mode = (zeroed) ? BUN_ALLOCATOR_MODE_RESIZE : BUN_ALLOCATOR_MODE_RESIZE_NON_ZEROED;

    if (mode &~ allocator->implemented_modes)
    {
        allocator->error = BUN_ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED;
        return NULL;
    }

    return allocator->proc( &allocator->data, &allocator->error,
                            mode,
                            size,
                            alignment, /*byte*/
                            ptr,
                            old_size
                          );
}

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
    offset = (uintptr_t)Bun_Align_formula(current_pointer, alignment) - (uintptr_t)arena->buffer;

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

void Bun_Arena_Free_AllBun_(Bun_Arena *arena)
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
    offset = (uintptr_t)Bun_Align_formula(current_pointer, alignment) - (uintptr_t)pool->buffer;

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
        offset = (uintptr_t)Bun_Align_formula(current_pointer, alignment) - (uintptr_t)pool->buffer;

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

Bun_String Bun_String_Alias(const char *cstring, Bun_U32 len)
{
    if (!len) len = (Bun_U32)strlen(cstring);
    return (Bun_String){
        .ptr = (char*)cstring,
        .len = len
    };
}

Bun_String Bun_String_Copy(const char *cstring, Bun_U32 len, Bun_Allocator *allocator)
{
    Bun_String string = {0};

    if (cstring == NULL) return string;

    if (!len) len = (Bun_U32)strlen(cstring);
    string = (Bun_String){
        .ptr = Bun_Allocator_Alloc(len, false, 1, allocator),
        .len = len
    };
    if (string.ptr == NULL) return string;

    memcpy((char *)string.ptr, cstring, len);
    return string;
}

Bun_String Bun_String_Duplicate(Bun_String string, Bun_Allocator *allocator)
{
    return Bun_String_Copy( string.ptr, string.len, allocator );
}

bool Bun_String_Is_Null_Terminated(Bun_String string)
{
    return string.ptr && string.ptr[string.len] == '\0';
}

#endif /*BUN_IMPLEMENTATION*/

#ifdef BUN_STRIP_PREFIX
#    define S8 Bun_S8
#    define S16 Bun_S16
#    define S32 Bun_S32
#    define S64 Bun_S64

#    define U8 Bun_U8
#    define U16 Bun_U16
#    define U32 Bun_U32
#    define U64 Bun_U64

#    define F32 Bun_F32
#    define F64 Bun_F64
#    define F128 Bun_F128
#    define String Bun_String
#    define Allocator_Error Bun_Allocator_Error
#        define ALLOCATOR_ERROR_NONE                 BUN_ALLOCATOR_ERROR_NONE 
#        define ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED BUN_ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED 
#        define ALLOCATOR_ERROR_OUT_OF_MEMORY        BUN_ALLOCATOR_ERROR_OUT_OF_MEMORY 
#        define ALLOCATOR_ERROR_INVALID_POINTER      BUN_ALLOCATOR_ERROR_INVALID_POINTER 
#        define ALLOCATOR_ERROR_INVALID_ARGUMENT     BUN_ALLOCATOR_ERROR_INVALID_ARGUMENT 
#        define ALLOCATOR_ERROR_UNKNOWN              BUN_ALLOCATOR_ERROR_UNKNOWN 
#        define _ALLOCATOR_ERROR_COUNT               _BUN_ALLOCATOR_ERROR_COUNT 
#    define Allocator_Mode Bun_Allocator_Mode
#        define ALLOCATOR_MODE_ALLOC             BUN_ALLOCATOR_MODE_ALLOC 
#        define ALLOCATOR_MODE_ALLOC_NON_ZEROED  BUN_ALLOCATOR_MODE_ALLOC_NON_ZEROED 
#        define ALLOCATOR_MODE_FREE              BUN_ALLOCATOR_MODE_FREE 
#        define ALLOCATOR_MODE_FREE_ALL          BUN_ALLOCATOR_MODE_FREE_ALL 
#        define ALLOCATOR_MODE_RESIZE            BUN_ALLOCATOR_MODE_RESIZE 
#        define ALLOCATOR_MODE_RESIZE_NON_ZEROED BUN_ALLOCATOR_MODE_RESIZE_NON_ZEROED 
#    define Allocator_Proc Bun_Allocator_Proc
#    define Allocator Bun_Allocator
#    define Arena Bun_Arena
#    define Dynamic_Arena Bun_Dynamic_Arena
#    define Allocator_Alloc Bun_Allocator_Alloc
#    define Allocator_Free Bun_Allocator_Free
#    define Allocator_Free_all Bun_Allocator_Free_all
#    define Allocator_Resize Bun_Allocator_Resize
#    define Allocator_NEW Bun_Allocator_NEW
#    define Arena_Init_From_Allocator Bun_Arena_Init_From_Allocator
#    define Arena_Deinit_From_Allocator Bun_Arena_Deinit_From_Allocator
#    define Arena_Alloc Bun_Arena_Alloc
#    define Arena_Resize Bun_Arena_Resize
#    define Arena_Free_All Bun_Arena_Free_All
#    define Dynamic_Arena_Init Bun_Dynamic_Arena_Init
#    define Dynamic_Arena_Deinit Bun_Dynamic_Arena_Deinit
#    define Dynamic_Arena_Alloc_Push Bun_Dynamic_Arena_Alloc_Push
#    define Dynamic_Arena_Alloc_Insert Bun_Dynamic_Arena_Alloc_Insert
#    define Dynamic_Arena_Resize Bun_Dynamic_Arena_Resize
#    define Dynamic_Arena_Free_All Bun_Dynamic_Arena_Free_All
#    define Dynamic_Arena_Free_Pools Bun_Dynamic_Arena_Free_Pools
#    define allocator_libc bun_allocator_libc
#    define Align_formula Bun_Align_formula
#    define String_Alias Bun_String_Alias
#    define String_Copy Bun_String String_Copy
#    define String_Duplicate Bun_String_Duplicate
#    define String_Is_Null_Terminated Bun_String_Is_Null_Terminated
#    ifndef BUN_NO_MACROS
#        define INTERNAL BUN_INTERNAL
#        define GLOBAL BUN_GLOBAL
#        define BIT_L BUN_BIT_L
#        define BIT_H BUN_BIT_H
#        define BIT BUN_BIT
#        define DEFER_LOOP BUN_DEFER_LOOP
#        define MIN BUN_MIN
#        define MAX BUN_MAX
#        define CLAMP BUN_CLAMP
#        define INT_FROM_PTR BUN_INT_FROM_PTR
#        define MEMBER BUN_MEMBER
#        define OFFSET_OF BUN_OFFSET_OF
#        define MEMBER_FROM_OFFSET BUN_MEMBER_FROM_OFFSET
#        define CAST_FROM_MEMBER BUN_CAST_FROM_MEMBER
#    endif /*ifndef BUN_NO_MACROS*/
#    define ALLOCATOR_DEFAULT_ALIGN BUN_ALLOCATOR_DEFAULT_ALIGN

#endif /*ifdef BUN_STRIP_PREFIX*/
