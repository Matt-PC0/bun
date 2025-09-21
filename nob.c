#define NOB_IMPLEMENTATION
#include "nob.h"

#define LIB_DIR "lib/"
#define INC_DIR "include/"
#define TEST_DIR "test/"
#define BIN_DIR "bin/"
#define SRC_DIR "src/"

#if defined(__GNUC__)
#   define CC "gcc"
#elif defined(__clang__)
#   define CC "clang"
#elif defined(_MSC_VER)
#   define CC "cl.exe"
#else
#   define CC "cc"
#endif

void build_bun_lib( Nob_Cmd *cmd )
{
    nob_cmd_append( cmd, CC, "-c", INC_DIR"bun.h", "-DBUN_IMPLEMENTATION", "--std=c89", "-o", LIB_DIR"bun.o" );
}
void build_test_bin( Nob_Cmd *cmd )
{
    nob_cmd_append( cmd, CC, TEST_DIR"test.c", "-I"INC_DIR, "--std=c89", "-ggdb", "-o", BIN_DIR"test.elf" );
}

bool path_has_single_char_extension_len(const char *path, size_t path_len, char extension)
{
    return (path_len >= 3
            &&  path[path_len-2] == '.'
            &&  path[path_len-1] == extension
           );
}
bool path_has_single_char_extension(const char *path, char extension)
{
    return path_has_single_char_extension_len(path, strlen(path), extension);
}

bool copy_file_contents( FILE *in, FILE *out )
{
    #define BUFFER_SIZE 1024
    char buffer[BUFFER_SIZE] = {0};
    size_t total_chars_written = 0;
    for (;/*ever*/;)
    {
        memset(buffer, 0, BUFFER_SIZE);
        size_t n_chars_read = fread( buffer, 1, BUFFER_SIZE-1, in);
        size_t buffer_len = strlen(buffer); //so dumb
        if (buffer_len == 0) break;
        size_t n_chars_written = fwrite( buffer, 1, buffer_len, out );
        if (n_chars_read == 0) break;
        if (n_chars_written != n_chars_read) return false;
        total_chars_written += n_chars_written;
    }
    nob_log(NOB_INFO, "wrote %fKB", (float)total_chars_written / 1000.0f);
    return true;
}

int strcmp_sort(const void *a, const void *b)
{
    const char **str_a = (const char **)a;
    const char **str_b = (const char **)b;
    return strcmp(*str_a, *str_b);
}
/* use quick sort */
void sort_file_paths( Nob_File_Paths *paths )
{
    qsort( paths->items, paths->count, sizeof(paths->items[0]), strcmp_sort );
}

#define FWRITE_STR(str, file) fwrite(str, 1, strlen(str), file)

bool copy_all_files_with_extension(Nob_File_Paths src_files, const char *src_dir, char extension, FILE *header_file)
{
    for (int i = 0; i < src_files.count; i++)
    {
        #define FILE_PATH_SIZE 1024
        char file_path[1024] = {0};
        if (strlen(src_dir) + strlen(src_files.items[0]) > FILE_PATH_SIZE) continue;
        strcpy(file_path, src_dir);
        strcat(file_path, src_files.items[i]);
        if (!path_has_single_char_extension(file_path, extension)) continue;

        FILE *source_file = fopen(file_path, "r");
        if (source_file == NULL)
        {
            nob_log(NOB_ERROR, "Couldnt open source file '%s' for reading %s\n", file_path, strerror(errno));
            return false;
        }

        FWRITE_STR("\n/*** -- ", header_file);
        FWRITE_STR(src_files.items[i], header_file);
        FWRITE_STR(" -- ***/\n", header_file);

        nob_log(NOB_INFO, "copying contents from '%s'", file_path);
        bool did_copy = copy_file_contents( source_file, header_file );
        fclose(source_file);
        if (!did_copy)
        {
            nob_log(NOB_ERROR, "failed to write to header file from source file '%s'\n", file_path);
            return false;
        }
    }
    return true;
}

bool generate_header(const char *header_file_path, const char *src_dir)
{
    bool result = false;

    Nob_File_Paths src_files = {0};
    if (!nob_read_entire_dir(src_dir, &src_files)) return false;
    sort_file_paths(&src_files);

    FILE *header_file = fopen(header_file_path, "w");
    if (header_file == NULL)
    {
        nob_log(NOB_ERROR, "Couldnt create or open file header file '%s' for writing %s\n", header_file_path, strerror(errno));
        return false;
    }

    FWRITE_STR("#ifndef BUN_H\n#define BUN_H\n", header_file);

    // first pass, headers
    if (!copy_all_files_with_extension(src_files, src_dir, 'h', header_file)) goto Defer_Exit;

    FWRITE_STR("#endif /*ifndef BUN_H*/\n", header_file);

    FWRITE_STR("#ifdef BUN_IMPLEMENTATION\n", header_file);

    //second pass, implementation files
    if (!copy_all_files_with_extension(src_files, src_dir, 'c', header_file)) goto Defer_Exit;

    FWRITE_STR("#endif /*ifndef BUN_IMPLEMENTATION*/\n", header_file);

    result = true;
Defer_Exit:
    fclose(header_file);
    return result;
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if ( !nob_mkdir_if_not_exists(LIB_DIR)
    ||   !nob_mkdir_if_not_exists(BIN_DIR)
    ||   !nob_mkdir_if_not_exists(INC_DIR)
    ) return 1;

    if (!generate_header(INC_DIR"bun.h", SRC_DIR)) return 1;

    Nob_Cmd cmd = {0};

    build_bun_lib( &cmd );
    if (!nob_cmd_run_sync_and_reset( &cmd )) return 1;

    build_test_bin( &cmd );
    if (!nob_cmd_run_sync_and_reset( &cmd )) return 1;

    nob_cmd_free(cmd);
    return 0;
}
