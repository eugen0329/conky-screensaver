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

typedef struct {
    unsigned long onIdleTimeout;
    timespec_t onIdleRefreshRate;
    timespec_t onBlankedRefreshRate;
    char display[BUF_SIZE];
} daemonConfigs_T;

void initDaemon();
pid_t startScreensaver();
void xscreensaverCommand(const char * cmd);
void waitIdle(unsigned long timeout, const timespec_t * refreshRate);
void waitXScreensaverUnblanked(const timespec_t * refreshRate);
pid_t runScrLocker();


const static daemonConfigs_T DEFAULTS = {
    .onIdleTimeout = MIN_TO_MSEC(1./60),
    .onIdleRefreshRate = { .tv_sec = 0, .tv_nsec = TO_NANO_SEC(0.2) },
    .onBlankedRefreshRate = { .tv_sec = 0, .tv_nsec = TO_NANO_SEC(0.2) },
    .display = "\":0.0\""
};

static XScreenSaverInfo *info;
static Display *display;
static daemonConfigs_T * configs;

int main(int argc, char *argv[])
{
    initDaemon();
    waitIdle(configs->onIdleTimeout, &configs->onIdleRefreshRate);
    printf("Specified idle time (%lu ms) is reached.\n", configs->onIdleTimeout);

    pid_t xscrPid = startScreensaver();
    xscreensaverCommand("-activate");

    waitXScreensaverUnblanked(&configs->onBlankedRefreshRate);
    puts("\n\nExiting XScreensaver");

    pid_t lockerPid = runScrLocker();
    while(true) {
        pid_t result = waitpid(lockerPid, NULL, WNOHANG);
        if (result == 0) {
            printf("scrlocker is alive\n");
        } else {
            printf("scrlocker is dead\n");
            break;
        }
    }



    kill(xscrPid, SIGKILL);
    XFree(info);
    XCloseDisplay(display);
    free(configs);

    return EXIT_SUCCESS;
}

void initDaemon()
{
    system("killall xscreensaver &>/dev/null");

    configs = malloc(sizeof(daemonConfigs_T));
    memcpy(configs, &DEFAULTS, sizeof(daemonConfigs_T));
    //if(configs->onIdleTimeout > ULONG_MAX) abortem("Idle timeout is too large");

    char setDispCmd[BUF_SIZE];
    snprintf(setDispCmd, sizeof(setDispCmd),
            "export DISPLAY=%s", configs->display);
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

pid_t runScrLocker()
{
    pid_t cpid = fork();
    if(cpid == FORK_SUCCESS) {
        execl("/usr/bin/slimlock", "/usr/bin/slimlock", NULL);
    } else if(cpid == FORK_ERR) 
        abortem("fork");

    return cpid;
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

