#define _DEFAULT_SOURCE
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
#include <gtk/gtk.h>

#include "helpers.h"

#define BUF_SIZE 256
#define FORK_SUCCESS 0
#define FORK_ERR  -1
#define RVAL_ERR  -1
#define DAEMON_IS_TERMINATED 1


#define FORCE_INLINE static inline __attribute__((always_inline))


typedef struct {
    unsigned long onIdleTimeout;
    timespec_t onIdleRefreshRate;
    timespec_t onBlankedRefreshRate;
    unsigned long onLockedIdleTimeout;
    timespec_t onLockedRefreshRate;
    char display[BUF_SIZE];
} daemonConfigs_t;

typedef enum { WSTILL_WAIT, WTIME_IS_OUT, WIS_UNLOCKED } waitRVal_t;

void initDaemon();
pid_t startScreensaver();
void xscreensaverCommand(char * cmd);
uint8_t waitIdle(unsigned long timeout, const timespec_t * refreshRate, int opts);
void waitXScreensaverUnblanked(const timespec_t * refreshRate);
pid_t runScrLocker();
uint8_t waitUnlocked(pid_t lockerPid, const timespec_t* refreshRate, unsigned long timeout);
void termhdl(int);
pid_t appendBackground();
FORCE_INLINE bool_t idleTimeIsOut(unsigned long timeout);

void (*currState)();
void onIdle();
void onBlanked();
void onLocked();

const static daemonConfigs_t DEFAULTS = {
    .onIdleTimeout = MIN_TO_MSEC(1./60),
    .onIdleRefreshRate = { .tv_sec = 0, .tv_nsec = TO_NANO_SEC(0.2) },
    .onBlankedRefreshRate = { .tv_sec = 0, .tv_nsec = TO_NANO_SEC(0.2) },
    .onLockedIdleTimeout =  MIN_TO_MSEC(1./60),
    .onLockedRefreshRate = { .tv_sec = 0, .tv_nsec = TO_NANO_SEC(0.2) },
    .display = "\":0.0\""
};

static XScreenSaverInfo *info;
static Display *display;
static daemonConfigs_t * configs;
bool_t isTerminated = false;
static pid_t bgPid;
static pid_t xscrPid;


int main(int argc, char *argv[])
{
    /*uint8_t waitStatus;*/
    currState = onIdle;

    initDaemon();
    xscrPid = startScreensaver();
    puts("Daemon is ready");

    while(true) {
        (*currState)();
    }

    puts("Release of resources");
    free(configs);
    kill(xscrPid, SIGKILL);
    XFree(info);
    XCloseDisplay(display);

    return EXIT_SUCCESS;
}

void onIdle()
{
    puts("Start again");
    waitIdle(configs->onIdleTimeout, &configs->onIdleRefreshRate, 0);
    bgPid = appendBackground();
    printf("Specified idle time (%lu ms) is reached.\n", configs->onIdleTimeout);
    currState = &onBlanked;
}

void onBlanked()
{
    xscreensaverCommand("-activate");
    waitXScreensaverUnblanked(&configs->onBlankedRefreshRate);
    puts("\n\nExiting XScreensaver");
    currState = &onLocked;
}

void onLocked()
{
    pid_t lockerPid;
    uint8_t waitResult;
    lockerPid = runScrLocker();

    waitResult = waitUnlocked(lockerPid, &configs->onLockedRefreshRate, configs->onLockedIdleTimeout);
    if(waitResult == WIS_UNLOCKED) {
        kill(bgPid, SIGKILL);
        currState = onIdle;
    } else {
        kill(lockerPid, SIGKILL);
        currState = onBlanked;
    }
}

uint8_t waitUnlocked(pid_t lockerPid, const timespec_t* refreshRate, unsigned long timeout)
{
    /*pid_t result;*/
    while(true) {
        nanosleep(refreshRate, NULL);
        if(idleTimeIsOut(timeout)) return WTIME_IS_OUT;
        if(waitpid(lockerPid, NULL, WNOHANG) != 0) return WIS_UNLOCKED;
        /*result = waitpid(lockerPid, NULL, WNOHANG);*/
        /*if (result == 0) {*/ /*printf("scrlocker is alive\n");*/
        /*} else {*/ /*printf("scrlocker is dead\n");*/ /*return WIS_UNLOCKED;*/ /*}*/
    }

    return 0;
}

pid_t appendBackground()
{
    GtkWidget *window;
    GdkColor color;
    pid_t cpid = fork();

    if(cpid == FORK_SUCCESS) {
        gtk_init(NULL, NULL);
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_widget_realize(window);
        gtk_window_fullscreen((GtkWindow*) window);
        color.red = 0x0;
        color.green = 0x0;
        color.blue = 0x0;
        gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &color);
        gtk_widget_show(window);
        gtk_main();
    } else if(cpid == FORK_ERR)
        abortem("fork");

    return cpid;
}

void termhdl(int notused)
{
    isTerminated = true;
}

void initDaemon()
{
    /*struct sigaction act;*/
    /*sigset_t mask;*/
    char setDispCmd[BUF_SIZE];

    system("killall xscreensaver &>/dev/null");

    configs = malloc(sizeof(daemonConfigs_t));
    memcpy(configs, &DEFAULTS, sizeof(daemonConfigs_t));

    snprintf(setDispCmd, sizeof(setDispCmd),
            "export DISPLAY=%s", configs->display);
    system(setDispCmd);

    if((display = XOpenDisplay(0)) == NULL)
        abortem("XOpenDisplay");
    if((info = XScreenSaverAllocInfo()) == NULL) {
        XCloseDisplay(display);
        abortem("XScreenSaverAllocInfo");
    }

    /*sigaddset(&mask, SIGINT);*/
    /*act.sa_handler = &termhdl;*/
    /*act.sa_flags = 0;*/
    /*act.sa_mask = mask;*/

    /*if(sigaction(SIGINT, &act, NULL) == RVAL_ERR ) abortem("sigaction");*/
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

uint8_t waitIdle(unsigned long timeout, const timespec_t * refreshRate, int opts)
{
    bool_t repeat = true;
    if(opts & WNOHANG) {
        repeat = false;
    }
    while(repeat) {
        /*if(isTerminated) return EXIT_FAILURE;*/
        if(idleTimeIsOut(timeout) == true) return WTIME_IS_OUT;
        nanosleep(refreshRate, NULL);
    }

    return WSTILL_WAIT;
}

FORCE_INLINE bool_t idleTimeIsOut(unsigned long timeout)
{
    XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
    if(info->idle >= timeout) return true;
    return false;
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
    FILE *pipe;
    uint8_t repeat = true;

    snprintf(cmd, sizeof(cmd),
        "[[ \"`xscreensaver-command -time`\" == *\"non-blanked\"* ]]"
        "&& echo -n %hu || echo -n %hu", true, false);

    while(repeat) {
        nanosleep(refreshRate, NULL);

        if ( (pipe = popen(cmd, "r")) == NULL) abortem("popen");
        system(cmd);
        fgets(cmdOutp, sizeof(cmdOutp), pipe);
        if(CH_TO_INT(cmdOutp[0]) == true) repeat = false;
        pclose(pipe);
    }
}

void xscreensaverCommand(char * cmd)
{
    char *const cargv[3] =
        {"/usr/bin/xscreensaver-command", cmd, NULL};
    pid_t cpid = fork();

    if(cpid == FORK_SUCCESS) {
        if(execvp(cargv[0], &cargv[0]) == RVAL_ERR) abortem("exec");
    } else if(cpid == FORK_ERR)
        abortem("fork");

    waitpid(cpid, NULL, 0);
}

