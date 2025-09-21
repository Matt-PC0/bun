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
