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
