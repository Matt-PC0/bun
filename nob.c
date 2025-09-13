#define NOB_IMPLEMENTATION
#include "nob.h"

#define LIB_DIR "lib/"
#define INC_DIR "include/"
#define TEST_DIR "test/"
#define BIN_DIR "bin/"


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

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if ( !nob_mkdir_if_not_exists(LIB_DIR)
    ||   !nob_mkdir_if_not_exists(BIN_DIR)
    ) return 1;

    Nob_Cmd cmd = {0};

    build_bun_lib( &cmd );
    if (!nob_cmd_run_sync_and_reset( &cmd )) return 1;

    build_test_bin( &cmd );
    if (!nob_cmd_run_sync_and_reset( &cmd )) return 1;

    nob_cmd_free(cmd);
    return 0;
}
