#ifndef INT_TYPES_H_JBTX63SD
#define INT_TYPES_H_JBTX63SD

#if __STDC_VERSION__ >= 199901L
# include <stdint.h>
typedef uint8_t         U8;
typedef uint16_t        U16;
typedef uint32_t        U32;
typedef uint64_t        U64;
#else
typedef unsigned char   U8;
typedef unsigned short  U16;
typedef unsigned int    U32;
typedef unsigned long   U64;
#endif

#endif /* end of include guard: INT_TYPES_H_JBTX63SD */
