#define BUN_IMPLEMENTATION
#define BUN_STRIP_PREFIX
#include "bun.h"

#include <stdio.h>

#define ASCII_START ' '
#define ASCII_END '~'
#define ASCII_RANGE (ASCII_END - ASCII_START)

int main(void)
{
    Arena arena;
    String str;
    int i;

    Arena_Init_From_Allocator( &arena, &allocator_libc, 1000, true, ALLOCATOR_DEFAULT_ALIGN );

    str.len = ASCII_RANGE;
    str.ptr = Arena_Alloc( str.len+1, true, ALLOCATOR_DEFAULT_ALIGN, &arena );

    for (i = 0; i < str.len; i++)
    {
        str.ptr[i] = ASCII_START + i;
    }

    printf("ascii range: '%s'\n", str);

    Arena_Deinit_From_Allocator(&arena, &allocator_libc);
    return 0;
}
