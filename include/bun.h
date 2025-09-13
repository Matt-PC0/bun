/*
BUN - Base lib for c programming.

Header acts as a single header lib although a static lib is also compileable.

STANDARD:
    C89 - should be compilable no matter what*

Defines:
    BUN_IMPLEMENTATION - Include Implementation code.
    BUN_NO_MACROS      - Exclude all macro #defines.

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

typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;
typedef int64_t  S64;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef float    F32;
typedef double   F64;

typedef U8 Byte

typedef struct
{
    const char *ptr;
    U32 len;
} String;

/* I'm using ODIN as refrence and it uses align_of*/
/* of course c < c11 doesnt have this, but the alignment*/
/* of a base type is equal it its size (I think).*/
/* In anycase this is 2*8 = 16 bytes on x86_64 which is what we want.*/
#define BUN_ALLOCATOR_DEFAULT_ALIGN 2 * sizeof(uintptr_t)

typedef U16 Allocator_Error;
enum
{
    ALLOCATOR_ERROR_NONE,
    ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED,
    ALLOCATOR_ERROR_OUT_OF_MEMORY,
    ALLOCATOR_ERROR_INVALID_POINTER,
    ALLOCATOR_ERROR_INVALID_ARGUMENT,
    ALLOCATOR_ERROR_UNKNOWN,
    _ALLOCATOR_ERROR_COUNT, /*If a custom allocator needs to have more errors have them relitive to this*/
};
typedef U8 Allocator_Mode;
enum
{
    ALLOCATOR_MODE_ALLOC             = (1<<0),
    ALLOCATOR_MODE_ALLOC_NON_ZEROED  = (1<<1),
    ALLOCATOR_MODE_FREE              = (1<<2),
    ALLOCATOR_MODE_FREE_ALL          = (1<<3),
    ALLOCATOR_MODE_RESIZE            = (1<<4),
    ALLOCATOR_MODE_RESIZE_NON_ZEROED = (1<<5),
};
typedef void *(*Allocator_Proc)(
                                void *allocator_data,
                                Allocator_Error *allocator_error,
                                Allocator_Mode mode,
                                U32 size,
                                U32 alignment,
                                void *old_memory,
                                U32 old_size
                               );
typedef struct
{
    Allocator_Proc proc;

    Allocator_Mode implemented_modes;
    void *data;
    Allocator_Error error;
} Allocator;

typedef struct
{
    Byte *buffer;
    U32 buffer_size;
    U32 offset;

} Arena

typedef struct
{
    Arena *pools;
    U32 pool_len;
    U32 pool_offset;

    U32  pool_size;
    bool pool_zeroed;
    U32  pool_alignment;
    Allocator *allocator;
} Dynamic_Arena;

void *Allocator_Alloc(U32 size, bool zeroed, U32 alignment, Allocator *allocator);
bool Allocator_Free(void *ptr, Allocator *allocator);
bool Allocator_Free_all(Allocator *allocator);
void *Allocator_Resize(void *ptr, U32 size, U32 old_size, bool zeroed, U32 alignment, Allocator *allocator);

#ifndef BUN_NO_MACROS
#define Allocator_NEW( T, A ) ((T*)Allocator_Alloc( sizeof(T), true, BUN_ALLOCATOR_DEFAULT_ALIGN, A ))
#endif

void Arena_Init_From_Allocator(Arena *arena, U32 buffer_size, bool zeroed, U32 alignment, Allocator *allocator);
void Arena_Deinit_From_Allocator(Arena *arena, Allocator *allocator);
void *Arena_Alloc(U32 size, bool zeroed, U32 alignment, Arena *arena);
void *Arena_Resize(void *old_memory, U32 size, U32 old_size, bool zeroed, U32 alignment, Arena *arena);
void  Arena_Free_All(Arena *arena);

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
bool Dynamic_Arena_Init( Dynamic_Arena *arena, Allocator *backing_allocator, U32 pool_size, bool pool_zeroed, U32 pool_alignment );
/*
Deinitialise dynamic_arena and free all pools

ARGS:
    arena - initialised arena
*/
void Dynamic_Arena_Deinit( Dynamic_Arena *arena);
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
void *Dynamic_Arena_Alloc_Push(U32 size, bool zeroed, U32 alignment, Dynamic_Arena *arena);
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
void *Dynamic_Arena_Alloc_Insert(U32 size, bool zeroed, U32 alignment, Dynamic_Arena *arena);
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
void *Dynamic_Arena_Resize(void *old_memory, U32 size, U32 old_size, bool zeroed, U32 alignment, Dynamic_Arena *arena);
/*
Free every allocation, but hold onto the allocated pools.
New allocations after a free_all will overwrite the old memory in the pools.

ARGS:
    arena      - an initialised dynamic arena
    zero_pools - set the memory in all the pools to zero (if you use ZII, prefer this to individual zeroed allocs)
*/
void Dynamic_Arena_Free_All(Dynamic_Arena *arena, bool zero_pools);
/*
Free every allocation, and free all but untill min_pools pools.
New allocations after a free_all will overwrite the old memory in the pools.

ARGS:
    arena      - an initialised dynamic arena
    min_pools  - number of pools to keep allocated. (individual objects allocated within these pools are still freed)
    zero_pools - set the memory in all the pools to zero (if you use ZII, prefer this to individual zeroed allocs)
*/
void Dynamic_Arena_Free_Pools(Dynamic_Arena *arena, U32 min_pools, bool zero_pools);

extern Allocator allocator_libc;


/*
Align to nearest *alignment* forward
*/
U32 Bun_Align_formula( U32 size, U32 alignment);

/*
Alias cstring as string; does no allocation/copy.

ARGS:
    cstring - NULL terminated string.
    len     - length of cstring or 0 to use strlen(cstring)
RETURN:
    string with .ptr == cstring and .len == *len* or (U32)strlen(cstring)
*/
String String_Alias(const char *cstring, U32 len);
/*
Copy cstring to a new string using the provided allocator.

ARGS:
    cstring   - NULL terminated string.
    len       - length of cstring or 0 to use strlen(cstring)
    allocator - A valid allocator that supports ALLOCATOR_MODE_ALLOC.
RETURN:
    string with .ptr == cstring and .len == *len* or (U32)strlen(cstring)
*/
String String_Copy(const char *cstring, U32 len, Allocator *allocator);
/*
Dublicate string using the provided allocator.

ARGS:
    string    - A valid bun string.
    allocator - A valid allocator that supports ALLOCATOR_MODE_ALLOC
RETURN:
    A copy of string
*/
String String_Duplicate(String string, Allocator *allocator);
/*
Check wether string.ptr is null terminated respecting string.len.

ARGS:
    string - A bun string.
RETURN:
    true if string.ptr is null terminated otherwise false
*/
bool String_Is_Null_Terminated(String string);

#ifndef BUN_NO_MACROS


/*only in c would one key work mean both internal and global*/
#define INTERNAL      static
#define GLOBAL        static

#define BIT_L(N) (1<<N)
#define BIT_H(N) (1ull<<N)
#define BIT(N) ( (N < 32) BIT_L(N) : BIT_H(N) )

#define DEFER_LOOP( begin, end ) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end)) 

#define MIN(A,B) (((A)<(B))?(A):(B))
#define MAX(A,B) (((A)>(B))?(A):(B))
#define CLAMP(A,X,B) (((X)<(A))?(A):((X)>(B))?(B):(X))

#define INT_FROM_PTR(ptr) ((uintptr_t)(ptr))

#define MEMBER(T,m)                   (((T*)0)->m)
#define OFFSET_OF(T,m)                INT_FROM_PTR(&MEMBER(T,m))
#define MEMBER_FROM_OFFSET(T,ptr,off) (T)((((U8 *)ptr)+(off)))
#define CAST_FROM_MEMBER(T,m,ptr)     (T*)(((U8*)ptr) - OFFSET_OF(T,m))

#endif /*BUN_NO_MACROS*/

#endif /*BUN_H*/

#define BUN_IMPLEMENTATION
#ifdef BUN_IMPLEMENTATION

U32 Bun_Align_formula( U32 size, U32 alignment)
{
    U32 result = size + alignment-1;
    return result - result % alignment;
}

void *Allocator_Libc_Proc(void *allocator_data,
                          Allocator_Error *allocator_error,
                          Allocator_Mode mode,
                          U32 size,
                          U32 alignment,
                          void *old_memory,
                          U32 old_size
                          )
{
    void * ptr;
    switch (mode)
    {
        case ALLOCATOR_MODE_ALLOC:
        case ALLOCATOR_MODE_ALLOC_NON_ZEROED:
            if (mode == ALLOCATOR_MODE_ALLOC) ptr = calloc( Bun_Align_formula(size, alignment), 1 );
            else                              ptr = malloc( Bun_Align_formula(size, alignment) );
            if (ptr == NULL && allocator_error != NULL)
            {
                *allocator_error = (errno == ENOMEM) ? ALLOCATOR_ERROR_OUT_OF_MEMORY : ALLOCATOR_ERROR_UNKNOWN;
            }
            return ptr;
        case ALLOCATOR_MODE_FREE:
            if (old_memory == NULL)
            {
                if (allocator_error != NULL) *allocator_error = ALLOCATOR_ERROR_INVALID_POINTER;
                return NULL;
            }
            free(old_memory);
            return old_memory;
        case ALLOCATOR_MODE_FREE_ALL:
            return NULL; /*unimplemented*/
        case ALLOCATOR_MODE_RESIZE:
            if (old_size == 0)
            {
                if (allocator_error != NULL) *allocator_error = ALLOCATOR_ERROR_INVALID_ARGUMENT;
                return NULL;
            }
            /* fallthrough */
        case ALLOCATOR_MODE_RESIZE_NON_ZEROED:
            if (old_memory == NULL || size == 0)
            {
                if (allocator_error != NULL) *allocator_error = ALLOCATOR_ERROR_INVALID_ARGUMENT;
                return NULL;
            }
            if (old_memory == NULL)
            {
                if (allocator_error != NULL) *allocator_error = ALLOCATOR_ERROR_INVALID_POINTER;
                return NULL;
            }

            ptr = realloc( old_memory, Bun_Align_formula(size, alignment) );

            if (ptr == NULL)
            {
                if (allocator_error != NULL)
                {
                    *allocator_error = (errno == ENOMEM) ? ALLOCATOR_ERROR_OUT_OF_MEMORY : ALLOCATOR_ERROR_UNKNOWN;
                }
                return NULL;
            }

            /* zero */
            if (mode == ALLOCATOR_MODE_RESIZE && size > old_size)
                memset(ptr + old_size, 0, size - old_size);

            return ptr;
        default:
            if (allocator_error != NULL) *allocator_error = ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED;
            return NULL;
    }
}

Allocator allocator_libc = (Allocator){
    .proc = &Allocator_Libc_Proc,
    .implemented_modes = ALLOCATOR_MODE_ALLOC
                       | ALLOCATOR_MODE_ALLOC_NON_ZEROED
                       | ALLOCATOR_MODE_FREE
                       | ALLOCATOR_MODE_RESIZE
                       | ALLOCATOR_MODE_RESIZE_NON_ZEROED,
    .data = NULL,
    .error = 0,
};

void *Allocator_Alloc(U32 size, bool zeroed, U32 alignment, Allocator *allocator)
{
    if (!allocator) return NULL;
    Allocator_Mode mode = (zeroed) ? ALLOCATOR_MODE_ALLOC : ALLOCATOR_MODE_ALLOC_NON_ZEROED;

    if (mode &~ allocator->implemented_modes)
    {
        allocator->error = ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED;
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
bool Allocator_Free(void *ptr, Allocator *allocator)
{
    if (!allocator) return NULL;

    if (mode &~ allocator->implemented_modes)
    {
        allocator->error = ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED;
        return false;
    }

    return allocator->proc( &allocator->data, &allocator->error,
                            ALLOCATOR_MODE_FREE,
                            0,
                            0,
                            ptr,
                            0
                          ) != NULL;

}
bool Allocator_Free_all(Allocator *allocator)
{
    if (!allocator) return NULL;

    if (mode &~ allocator->implemented_modes)
    {
        allocator->error = ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED;
        return NULL;
    }

    return allocator->proc( &allocator->data, &allocator->error,
                            ALLOCATOR_MODE_FREE_ALL,
                            0,
                            0,
                            NULL,
                            0
                          ) != NULL;
}
void *Allocator_Resize(void *ptr, U32 size, U32 old_size, bool zeroed, U32 alignment, Allocator *allocator)
{
    if (!allocator) return NULL;
    Allocator_Mode mode = (zeroed) ? ALLOCATOR_MODE_RESIZE : ALLOCATOR_MODE_RESIZE_NON_ZEROED;

    if (mode &~ allocator->implemented_modes)
    {
        allocator->error = ALLOCATOR_ERROR_MODE_NOT_IMPLEMENTED;
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

void Arena_Init_From_Allocator(Arena *arena, U32 buffer_size, bool zeroed, U32 alignment, Allocator *allocator)
{
    arena->buffer = Allocator_Alloc(size, zeroed, alignment, allocator);
    arena->buffer_size = size;
    arena->offset = 0;
}
void Arena_Deinit_From_Allocator(Arena *arena, Allocator *allocator)
{
    Allocator_Free(arena, allocator);
    *arena = {0};
}
void *Arena_Alloc(U32 size, bool zeroed, U32 alignment, Arena *arena)
{
    uintptr_t current_pointer, offset;
    void *ptr;

    current_pointer = (uintptr_t)arena->buffer + (uintptr_t)arena->offset;
    offset = (uintptr_t)Bun_Align_formula(current_pointer, alignment) - (uintptr_t)arena->buffer;

    if ( offset + size > arena->buffer_size ) return NULL;

    void *ptr = &arena->buffer[offset];
    arena->offset = offset + size;

    if (zeroed) memset(ptr, 0, size);

    return ptr;
}

void *Arena_Resize(void *old_memory, U32 size, U32 old_size, bool zeroed, U32 alignment, Arena *arena)
{
    uintptr_t offset, old_memory_offset;

    if (old_memory == NULL
    || old_size == 0
    || (uintptr_t)old_memory < (uintptr_t)arena.buffer
    || (uintptr_t)old_memory + size >= (uintptr_t)arena.buffer + arena.buffer_size
    ) return NULL;

    if (size < old_size)
    {
        arena->offset -= old_size - size;
        return ptr;
    }

    old_memory_offset = (uintptr_t)old_memory - (uintptr_t)arena.buffer;
    offset = old_memory_offset + (uintptr_t)old_size;
    if (offset == (uintptr_t)arena.buffer_offset)
    {
        offset = old_memory_offset + size;
        if (offset > arena->buffer_size) return NULL;
        buffer.offset = offset;
        return old_memory;
    }
    else
    {
        void *new_memory = Arena_Alloc(size, zeroed, alignment, arena);
        if (new_memory == NULL) return NULL;
        return memmove(new_memory, old_memory, size);
    }
}

void Arena_Free_All(Arena *arena)
{
    arena.offset = 0;
}

bool Dynamic_Arena_Init( Dynamic_Arena *arena, Allocator *backing_allocator, U32 pool_size, bool pool_zeroed, U32 pool_alignment )
{
    static const Allocator_Mode required_modes = ALLOCATOR_MODE_ALLOC
                                               | ALLOCATOR_MODE_ALLOC_NON_ZEROED
                                               | ALLOCATOR_MODE_RESIZE
                                               | ALLOCATOR_MODE_FREE;
    if (!arena || !backing_allocator || !Pool_Size
    || required_modes &~ backing_allocator->implemented_modes
    ) return false;

    arena->allocator      = backing_allocator;
    arena->pool_size      = pool_size;
    arena->pool_zeroed    = pool_zeroed;
    arena->pool_alignment = pool_alignment;
    arena->pool_offset    = 0;

    arena->pool_len = 8
    arena->pools = Allocator_Alloc(sizeof(Arena)*arena->pool_len, true, BUN_ALLOCATOR_DEFAULT_ALIGN, backing_allocator);
    if (arena->pools == NULL) return false;

    /* this is actually not a good idea, just let the first allocation handle it.
    arena->pools[0].buffer = Allocator_Alloc(pool_size, pool_zeroed, pool_alignment, backing_allocator);
    arena->pools[0].offset = 0;
    */
}
void Dynamic_Arena_Deinit( Dynamic_Arena *arena )
{
    if (!arena || !arena->allocator || !arena->pools) return;

    int i;
    for ( i = 0; i < arena->pool_len; i++ )
    {
        if (arena->pools[i].buffer == NULL) break;
        Allocator_Free(arena->pools[i].buffer, arena->backing_allocator)
    }
    Allocator_Free(arena->pools, arena->backing_allocator)
}

void *Dynamic_Arena_Alloc_Push(U32 size, bool zeroed, U32 alignment, Dynamic_Arena *arena)
{
    uintptr_t current_pointer, offset;
    void *ptr;
    Arena_Pool *pool;

    /*Im blanking on if `type x = y[]` is a copy or not, I think it is, but have no internet to check*/
    pool = &arena->pools[arena->pool_offset];

    current_pointer = (uintptr_t)pool->buffer + (uintptr_t)pool->offset;
    offset = (uintptr_t)Bun_Align_formula(current_pointer, alignment) - (uintptr_t)pool->buffer;

    if ( offset + size > arena->buffer_size )
    {
        U32 pool_size = (arena->pool_size > size) ? arena->pool_size : size;

        if (pool->buffer != NULL) arena->pool_offset += 1;

        if (arena->pool_offset >= arena->pool_len)
        {
            U32 old_size = sizeof(Arena_Pool)*arena->pool_len;
            Arena_Pool *new_ptr;
            
            arena->pool_len += 8;
            new_ptr = Allocator_Resize( arena->pools,
                                        sizeof(Arena_Pool)*arena->pool_len, old_size,
                                        true, BUN_ALLOCATOR_DEFAULT_ALIGN, arena->allocator);
            if (!new_ptr)
                return NULL;
            arena->pools = new_ptr;

        }
        pool = &arena->pools[arena->pool_offset];
        offset = 0;
        Arena_Init_From_Allocator( pool, pool_size, arena->pool_zeroed, arena->pool_alignment );
        if (pool.buffer == NULL) return NULL;
    }

    ptr = &pool->buffer[offset];
    pool->offset = offset + size;

    if (zeroed) memset(ptr, 0, size);

    return ptr;
}
void *Dynamic_Arena_Alloc_Insert(U32 size, bool zeroed, U32 alignment, Dynamic_Arena *arena)
{
    uintptr_t current_pointer, offset;
    void *ptr;
    Arena_Pool *pool;

    if (size > arena->pool_size) return Dynamic_Arena_Alloc_Push(size, zeroed, alignment, arena);

    int i
    for (i = 0; i < arena->pool_len; i++)
    {
        pool = &arena->pools[i];

        current_pointer = (uintptr_t)pool->buffer + (uintptr_t)pool->offset;
        offset = (uintptr_t)Bun_Align_formula(current_pointer, alignment) - (uintptr_t)pool->buffer;

        if ( offset + size > arena->buffer_size ) continue;

        ptr = &pool->buffer[offset];
        pool->offset = offset + size;

        if (zeroed) memset(ptr, 0 size);

        return ptr;
    }
    /* if we reach here there are no gaps to fill */
    return Dynamic_Arena_Alloc_Push(size, zeroed, alignment, arena)
}
void *Dynamic_Arena_Resize(void *old_memory, U32 size, U32 old_size, bool zeroed, U32 alignment, Dynamic_Arena *arena)
{
    uintptr_t current_pointer, offset;
    void *ptr;
    Arena_Pool *pool;
    int i;

    for (i = 0; i < arena->pool_offset; i++)
    {
        pool = &arena->pools[i];
        if (old_memory < pool->buffer || old_memory >= pool->buffer + arena->buffer_size)
            continue;
        /* pool found */
        offset = old_memory - pool->buffer;
        if (offset == pool->offset - old_size) /*Is on the end*/
        {
            if (offset + size >= pool->buffer_size)
            {
                buffer->offset = offset;
                ptr = Dynamic_Arena_Alloc_Push(size, zeroed, alignment, arena);
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
            ptr = Dynamic_Arena_Alloc_Push(size, zeroed, alignment, arena);
            if (ptr == NULL) return NULL;
            return memmove(ptr, old_memory, size);
        }

    }
    /* If here is reached old_memory is not a valid pointer */
    return NULL;
}
void Dynamic_Arena_Free_All(Dynamic_Arena *arena, bool zero_pools)
{
    if (arena == NULL) return;
    int i;
    for (i = 0; i < arena->pool_len; i++)
    {
        Arena *pool = &arena->pools[i];
        pool->offset = 0;
        if (zero_pools) memset(pool->buffer, 0, pool->buffer_size);
    }
    arena->pool_offset = 0;
}
void Dynamic_Arena_Free_Pools(Dynamic_Arena *arena, U32 min_pools, bool zero_pools)
{
    if (arena == NULL) return;

    if (arena->pool_len < min_pools) Dynamic_Arena_Free_All(arena, zero_pools);

    int i;
    for (i = 0; i < arena->pool_len; i++)
    {
        Arena *pool = &arena->pools[i];
        pool->offset = 0;
        if (i >= min_pools) Allocator_Free(pool->buffer, arena->allocator);
        else if (zero_pools) memset(pool->buffer, 0, pool->buffer_size);
    }
    arena->pools Allocator_Resize( arena->pools,
                                   sizeof(Arena)*min_pools, sizeof(Arena)*arena->pool_len,
                                   false, BUN_ALLOCATOR_DEFAULT_ALIGN, arena->allocator);
    arena->pool_len = min_pools;
    arena->pool_offset = 0;
}

String String_Alias(const char *cstring, U32 len)
{
    if (!len) len = (U32)strlen(cstring);
    return (String){
        .ptr = cstring,
        .len = len
    };
}

String String_Copy(const char *cstring, U32 len, Allocator *allocator)
{
    String string = {0};

    if (cstring == NULL) return string;

    if (!len) len = (U32)strlen(cstring);
    string = (String){
        .ptr = Allocator_Alloc(len, false, 1, allocator),
        .len = len
    };
    if (string.ptr == NULL) return string;

    memcpy((char *)string.ptr, cstring, len);
    return string;
}

String String_Duplicate(String string, Allocator *allocator)
{
    return Bun_String_Copy( string.ptr, string.len, allocator );
}

bool String_Is_Null_Terminated(String string)
{
    return string.ptr && string.ptr[string.len] == '\0';
}

#endif /*BUN_IMPLEMENTATION*/
