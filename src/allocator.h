#define BUN_ALLOCATOR_DEFAULT_ALIGN 2 * sizeof(uintptr_t)
#ifndef BUN_NO_MACROS
#define Bun_Allocator_NEW( T, A ) ((T*)Bun_Allocator_Alloc( sizeof(T), true, BUN_ALLOCATOR_DEFAULT_ALIGN, A ))
#endif

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


void *Bun_Allocator_Alloc(Bun_U32 size, bool zeroed, Bun_U32 alignment, Bun_Allocator *allocator);
bool Bun_Allocator_Free(void *ptr, Bun_Allocator *allocator);
bool Bun_Allocator_Free_all(Bun_Allocator *allocator);
void *Bun_Allocator_Resize(void *ptr, Bun_U32 size, Bun_U32 old_size, bool zeroed, Bun_U32 alignment, Bun_Allocator *allocator);

/*
Align to nearest *alignment* forward
*/
uintptr_t Bun_Align_Formula( uintptr_t size, Bun_U32 alignment);

extern Bun_Allocator bun_allocator_libc;

#ifdef BUN_STRIP_PREFIX
#    define ALLOCATOR_DEFAULT_ALIGN BUN_ALLOCATOR_DEFAULT_ALIGN
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
#    define Allocator_Alloc Bun_Allocator_Alloc
#    define Allocator_Free Bun_Allocator_Free
#    define Allocator_Free_all Bun_Allocator_Free_all
#    define Allocator_Resize Bun_Allocator_Resize
#    define Allocator_NEW Bun_Allocator_NEW
#    define Align_Formula Bun_Align_Formula
#    define allocator_libc bun_allocator_libc
#endif /*ifdef BUN_STRIP_PREFIX*/
