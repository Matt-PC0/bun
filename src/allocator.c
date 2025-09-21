#include <stdlib.h>
#include <errno.h>
#include <string.h>

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
            if (mode == BUN_ALLOCATOR_MODE_ALLOC) ptr = calloc( Bun_Align_Formula(size, alignment), 1 );
            else                              ptr = malloc( Bun_Align_Formula(size, alignment) );
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

            ptr = realloc( old_memory, Bun_Align_Formula(size, alignment) );

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


uintptr_t Bun_Align_Formula( uintptr_t size, Bun_U32 alignment)
{
    uintptr_t result = size + (uintptr_t)alignment-1;
    return result - result % (uintptr_t)alignment;
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

