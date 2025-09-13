# Bún
Bún is a Base lib for c programming.

# Standard
C89 - should hopefully be compilable with most compilers.

# Provides
- U8-64         - unsigned type aliases.
- S8-64         - signed type aliases.
- String        - String with length.
- Allocator     - A generic allocator interface.
- Arena         - A fixed size arena.
- Dynamic_Arena - A dynamically sized arena.

# Usage
## single header file
```c
#define BUN_IMPLEMENTATION
#include "bun.h"
int main(void)
{
    //...
}
```
```sh
cc main.c -I<path-to-bun.h-dir>
```

# Disclaimer
This is not thoroughly tested, it may have horrible memory bugs that Will ruin your day.

# Refrances
- https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
- https://pkg.odin-lang.org/core/mem/
- https://github.com/EpicGamesExt/raddebugger/tree/master
