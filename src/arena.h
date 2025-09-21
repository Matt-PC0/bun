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

#ifdef BUN_STRIP_PREFIX
#    define Arena Bun_Arena
#    define Dynamic_Arena Bun_Dynamic_Arena
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
#endif /*ifdef BUN_STRIP_PREFIX*/
