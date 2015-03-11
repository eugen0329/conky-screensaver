#ifndef HELPERS_H_EMYJVGR0
#define HELPERS_H_EMYJVGR0

#include <stdlib.h>
#include <stdio.h>

#define TO_NANO_SEC( x )  ( x * 1000000000 )
#define MIN_TO_MSEC( x )  ( x * 60 * 1000 )
#define CH_TO_INT( x )  ( x - '0' )

typedef struct timespec timespec_t;
typedef enum { false = 0, true = 1} bool_t;

void abortem(const char * msg);

#endif /* end of include guard: HELPERS_H_EMYJVGR0 */
