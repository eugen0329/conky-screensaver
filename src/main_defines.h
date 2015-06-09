#ifndef MAIN_DEFINES_H_AYRHVXKS
#define MAIN_DEFINES_H_AYRHVXKS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>
#include <limits.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <ctype.h>

#include <X11/extensions/scrnsaver.h>
#include <gtk/gtk.h>

#include "int_types.h"
#include "inline.h"
#include "helpers.h"

#include <time.h>   /* usleep */

#define BUF_SIZE 256

typedef struct timespec timespec_t;

typedef U64 onIdleTimeout_t;
typedef timespec_t onIdleRefreshRate_t;
typedef U64 onLockedTimeout_t;
typedef timespec_t onLockedRefreshRate_t;
typedef timespec_t onBlankedRefreshRate_t;

typedef struct {
    U64        onIdleTimeout;
    timespec_t    onIdleRefreshRate;

    U64      onLockedIdleTimeout;
    timespec_t  onLockedRefreshRate;

    timespec_t onBlankedRefreshRate;
    char                   display[BUF_SIZE];
} daemonConfigs_t;

typedef enum { WSTILL_WAIT, WTIME_IS_OUT, WIS_UNLOCKED } waitRVal_t;

timespec_t  NULL_TIMESPEC = {0};
const char WRONG_ARG_ERR[] = "Error while parsing the arguments";



#endif /* end of include guard: MAIN_DEFINES_H_AYRHVXKS */
