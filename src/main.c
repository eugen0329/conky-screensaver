#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>
#include <limits.h>
#include <stdint.h>  /* uint8_t */
#include <time.h>    /* usleep */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
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

void initDaemon();
pid_t startScreensaver();
void xscreensaverCommand(const char * cmd);
void waitIdle(unsigned long timeout, const timespec_t * refreshRate);
void waitXScreensaverUnblanked(const timespec_t * refreshRate);

static const char DEFAULT_DISPLAY[] = "\":0.0\"";

static XScreenSaverInfo *info;
static Display *display;

int main(int argc, char *argv[])
{
    initDaemon();

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


    waitIdle(onIdleTimeout, &onIdleRefreshRate);
    printf("Specified idle time (%lu ms) is reached.\n", onIdleTimeout);

    pid_t xscrPid = startScreensaver();
    xscreensaverCommand("-activate");

    waitXScreensaverUnblanked(&onBlankedRefreshRate);
    puts("\n\nExiting XScreensaver");
    kill(xscrPid, SIGKILL);


    XFree(info);
    XCloseDisplay(display);

    return EXIT_SUCCESS;
}

void initDaemon()
{
    system("killall xscreensaver 2>/dev/null");

    char setDispCmd[BUF_SIZE];
    snprintf(setDispCmd, sizeof(setDispCmd), "export DISPLAY=%s", DEFAULT_DISPLAY);
    system(setDispCmd);

    if((display = XOpenDisplay(0)) == NULL)
        abortem("XOpenDisplay");
    if((info = XScreenSaverAllocInfo()) == NULL) {
        XCloseDisplay(display);
        abortem("XScreenSaverAllocInfo");
    }
}

pid_t startScreensaver()
{
    pid_t cpid = fork();

    if(cpid == FORK_SUCCESS) {
        char * cargv[] = {"/usr/bin/xscreensaver", "-no-splash", NULL};
        if(execvp(cargv[0], &cargv[0]) == RVAL_ERR) abortem("exec");
    } else if(cpid == FORK_ERR)
        abortem("fork");

    return cpid;
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
    char cmdOutp[BUF_SIZE];
    char cmd[BUF_SIZE];

    snprintf(cmd, sizeof(cmd),
        "[[ \"`xscreensaver-command -time`\" == *\"non-blanked\"* ]]"
        "&& echo -n %hu || echo -n %hu", true, false);

    FILE *pipe;
    uint8_t repeat = true;
    while(repeat) {
        nanosleep(refreshRate, NULL);

        if ( (pipe = popen(cmd, "r")) == NULL) abortem("popen");
        system(cmd);
        fgets(cmdOutp, sizeof(cmdOutp), pipe);
        if(CH_TO_INT(cmdOutp[0]) == true) repeat = false;
        pclose(pipe);
    }
}

void xscreensaverCommand(const char * cmd)
{
    pid_t cpid = fork();

    if(cpid == FORK_SUCCESS) {
        char *const cargv[3] =
            {"/usr/bin/xscreensaver-command", cmd, NULL};
        if(execvp(cargv[0], &cargv[0]) == RVAL_ERR) abortem("exec");
    } else if(cpid == FORK_ERR)
        abortem("fork");

    waitpid(cpid, NULL, 0);
}

