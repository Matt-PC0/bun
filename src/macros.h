#ifndef BUN_NO_MACROS

/*only in c would one key work mean both internal and global*/
#define BUN_INTERNAL      static
#define BUN_GLOBAL        static

#define BUN_BIT_L(N) (1<<N)
#define BUN_BIT_H(N) (1ull<<N)
#define BUN_BIT(N) ( (N < 32) BIT_L(N) : BIT_H(N) )

#define BUN_DEFER_LOOP( begin, end ) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end)) 

#define BUN_MIN(A,B) (((A)<(B))?(A):(B))
#define BUN_MAX(A,B) (((A)>(B))?(A):(B))
#define BUN_CLAMP(A,X,B) (((X)<(A))?(A):((X)>(B))?(B):(X))

#define BUN_INT_FROM_PTR(ptr) ((uintptr_t)(ptr))

#define BUN_MEMBER(T,m)                   (((T*)0)->m)
#define BUN_OFFSET_OF(T,m)                INT_FROM_PTR(&MEMBER(T,m))
#define BUN_MEMBER_FROM_OFFSET(T,ptr,off) (T)((((U8 *)ptr)+(off)))
#define BUN_CAST_FROM_MEMBER(T,m,ptr)     (T*)(((U8*)ptr) - OFFSET_OF(T,m))


#ifdef BUN_STRIP_PREFIX
#    define INTERNAL BUN_INTERNAL
#    define GLOBAL BUN_GLOBAL
#    define BIT_L BUN_BIT_L
#    define BIT_H BUN_BIT_H
#    define BIT BUN_BIT
#    define DEFER_LOOP BUN_DEFER_LOOP
#    define MIN BUN_MIN
#    define MAX BUN_MAX
#    define CLAMP BUN_CLAMP
#    define INT_FROM_PTR BUN_INT_FROM_PTR
#    define MEMBER BUN_MEMBER
#    define OFFSET_OF BUN_OFFSET_OF
#    define MEMBER_FROM_OFFSET BUN_MEMBER_FROM_OFFSET
#    define CAST_FROM_MEMBER BUN_CAST_FROM_MEMBER
#endif /*ifdef BUN_STRIP_PREFIX*/

#endif /*BUN_NO_MACROS*/
