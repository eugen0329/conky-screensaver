#ifndef HELPERS_H_EMYJVGR0
#define HELPERS_H_EMYJVGR0

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <libnotify/notify.h>

#define SEC2NANOSEC( x )  ( x * 1000000000 )
#define MIN2MSEC( x )  ( x * 60 * 1000 )
#define CH2INT( x )  ( x - '0' )

typedef struct timespec timespec_t;
typedef enum { false = 0, true = 1} bool_t;

void abortem(const char * msg);
void abortWithNotif(const char * msg);

#endif /* end of include guard: HELPERS_H_EMYJVGR0 */

