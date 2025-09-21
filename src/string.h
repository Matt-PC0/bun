typedef struct
{
    char *ptr;
    Bun_U32 len;
} Bun_String;

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


#ifdef BUN_STRIP_PREFIX
#    define String Bun_String
#    define String_Alias Bun_String_Alias
#    define String_Copy Bun_String String_Copy
#    define String_Duplicate Bun_String_Duplicate
#    define String_Is_Null_Terminated Bun_String_Is_Null_Terminated
#endif /*ifdef BUN_STRIP_PREFIX*/
