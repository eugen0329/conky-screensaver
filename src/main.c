#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>
#include <limits.h>
#include <stdint.h>  /* uint8_t */
#include <time.h>    /* usleep */

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <X11/extensions/scrnsaver.h>

#include "helpers.h"

#define FORK_SUCCESS 0
#define FORK_ERR  -1
#define RVAL_ERR  -1
#define TO_NANO_SEC( x )  ( x * 1000000000 )
#define MIN_TO_MSEC( x )  ( x * 60 * 1000 )

typedef struct timespec timespec_t;
typedef enum { false = 0, true = 1} bool_t;

static XScreenSaverInfo *info;
static Display *display;

void waitIdle(unsigned long timeout, const timespec_t * checkRate);

int main(int argc, char *argv[])
{
    unsigned long onIdleTimeout = MIN_TO_MSEC(2./60);
    timespec_t onIdleRefreshRate = {
        .tv_sec = 0,
        .tv_nsec = TO_NANO_SEC(0.2)
    };
    if(onIdleTimeout > ULONG_MAX) abortem("Idle timeout is too large");


    if(display == NULL && (display = XOpenDisplay(0)) == NULL)
        abortem("XOpenDisplay");
    if(info == NULL && (info = XScreenSaverAllocInfo()) == NULL)
        abortem("XOpenDisplay");

    waitIdle(onIdleTimeout, &onIdleRefreshRate);
    printf("Specified idle time (%lu ms) is reached.\n", onIdleTimeout);

    XFree(info);
    XCloseDisplay(display);

    return EXIT_SUCCESS;
}


void waitIdle(unsigned long timeout, const timespec_t * checkRate)
{
    do {
        nanosleep(checkRate, NULL);
        XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
    } while(info->idle < timeout);
}


