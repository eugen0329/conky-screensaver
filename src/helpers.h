#ifndef HELPERS_H_EMYJVGR0
#define HELPERS_H_EMYJVGR0

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h> /* uint8_t */
#include <libnotify/notify.h>

#include "int_types.h"

#ifdef DEBUG /* define debug output functions */
#   define DPUTS(str) puts(str);
#   define DPRINTF(...) printf(__VA_ARGS__);
#else /* just eat up the output functions if release */
#   define DPRINTF(...)
#   define DPUTS(str)
#endif

#define SEC2NANOSEC( x )  ( x * 1000000000 )
#define MIN2MSEC( x )  ( x * 60 * 1000 )
#define CH2INT( x )  ( x - '0' )


typedef struct timespec timespec_t;
typedef enum { false = 0, true = 1} bool_t;

void abortem(const char * msg);
void abortWithNotif(const char * msg);

uint8_t parseULong(const char * from, U64* to);

void free2(void** ptr, U64 size);

#endif /* end of include guard: HELPERS_H_EMYJVGR0 */

