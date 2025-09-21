#include <stdint.h>
#include <stdbool.h>

typedef int8_t   Bun_S8;
typedef int16_t  Bun_S16;
typedef int32_t  Bun_S32;
typedef int64_t  Bun_S64;

typedef uint8_t  Bun_U8;
typedef uint16_t Bun_U16;
typedef uint32_t Bun_U32;
typedef uint64_t Bun_U64;

/*XXX: x86_64 only */
typedef float       Bun_F32;
typedef double      Bun_F64;
typedef long double Bun_F128;

typedef Bun_U8 Bun_Byte;


#ifdef BUN_STRIP_PREFIX
#    define S8 Bun_S8
#    define S16 Bun_S16
#    define S32 Bun_S32
#    define S64 Bun_S64

#    define U8 Bun_U8
#    define U16 Bun_U16
#    define U32 Bun_U32
#    define U64 Bun_U64

#    define F32 Bun_F32
#    define F64 Bun_F64
#    define F128 Bun_F128
#    define Byte Bun_Byte
#endif /*ifdef BUN_STRIP_PREFIX*/
