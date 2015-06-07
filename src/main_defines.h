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
    onIdleTimeout_t        onIdleTimeout;
    onIdleRefreshRate_t    onIdleRefreshRate;

    onLockedTimeout_t      onLockedIdleTimeout;
    onLockedRefreshRate_t  onLockedRefreshRate;

    onBlankedRefreshRate_t onBlankedRefreshRate;
    char                   display[BUF_SIZE];
} daemonConfigs_t;

typedef enum { WSTILL_WAIT, WTIME_IS_OUT, WIS_UNLOCKED } waitRVal_t;

timespec_t  NULL_TIMESPEC = {0};
const char WRONG_ARG_ERR[] = "Error while parsing the arguments";

const daemonConfigs_t DEFAULTS = {
    .onIdleTimeout = MIN2MSEC(1.5 / 60),
    .onLockedIdleTimeout = MIN2MSEC(2. / 60),
    .onIdleRefreshRate = {.tv_sec = 1, .tv_nsec = SEC2NANOSEC(0.2)},
    .onBlankedRefreshRate = {.tv_sec = 0, .tv_nsec = SEC2NANOSEC(0.2)},
    .onLockedRefreshRate = {.tv_sec = 0, .tv_nsec = SEC2NANOSEC(0.2)},
    .display = "\":0.0\""};


#endif /* end of include guard: MAIN_DEFINES_H_AYRHVXKS */
