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

#define BUF_SIZE 256
#define FORK_SUCCESS 0
#define FORK_ERR  -1
#define RVAL_ERR  -1

#define TO_NANO_SEC( x )  ( x * 1000000000 )
#define MIN_TO_MSEC( x )  ( x * 60 * 1000 )
#define CH_TO_INT( x )  ( x - '0' )

typedef struct timespec timespec_t;
typedef enum { false = 0, true = 1} bool_t;

void waitIdle(unsigned long timeout, const timespec_t * refreshRate);
void waitXScreensaverUnblanked(const timespec_t * refreshRate);

static XScreenSaverInfo *info;
static Display *display;

int main(int argc, char *argv[])
{
    unsigned long onIdleTimeout = MIN_TO_MSEC(1./60);
    timespec_t onIdleRefreshRate = {
        .tv_sec = 0,
        .tv_nsec = TO_NANO_SEC(0.2)
    };
    timespec_t onBlankedRefreshRate = {
        .tv_sec = 0,
        .tv_nsec = TO_NANO_SEC(0.2)
    };
    if(onIdleTimeout > ULONG_MAX) abortem("Idle timeout is too large");


    if((display = XOpenDisplay(0)) == NULL)
        abortem("XOpenDisplay");
    if((info = XScreenSaverAllocInfo()) == NULL) {
        XCloseDisplay(display);
        abortem("XScreenSaverAllocInfo");
    }

    waitIdle(onIdleTimeout, &onIdleRefreshRate);
    printf("Specified idle time (%lu ms) is reached.\n", onIdleTimeout);

    system("killall xscreensaver");
    system("export DISPLAY=\":0.0\"; xscreensaver -no-splash &");
    system("xscreensaver-command -activate");

    waitXScreensaverUnblanked(&onBlankedRefreshRate);
    puts("\n\nExiting XScreensaver");


    system("killall xscreensaver");
    XFree(info);
    XCloseDisplay(display);

    return EXIT_SUCCESS;
}

void waitIdle(unsigned long timeout, const timespec_t * refreshRate)
{
    do {
        nanosleep(refreshRate, NULL);
        XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
    } while(info->idle < timeout);
}

void waitXScreensaverUnblanked(const timespec_t * refreshRate)
{
    const char cmd[] =
        "[[ \"$(xscreensaver-command -time)\" == *\"non-blanked\"* ]] && echo -n 0 || echo -n 1";
    char output[BUF_SIZE];

    FILE *pipe;
    uint8_t repeat = true;
    while(repeat) {
        nanosleep(refreshRate, NULL);
        if ( (pipe = popen(cmd, "r")) == NULL) abortem("popen");
        system(cmd);
        fgets(output, sizeof(output), pipe);
        if(CH_TO_INT(output[0]) == 0) repeat = false;
        pclose(pipe);
    }
}

