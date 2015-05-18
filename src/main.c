#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>
#include <limits.h>
#include <stdint.h> /* uint8_t */
#include <time.h>   /* usleep */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <getopt.h>
#include <ctype.h>

#include <X11/extensions/scrnsaver.h>
#include <libconfig.h>
#include <gtk/gtk.h>


#include "helpers.h"
#include "inline.h"

#define BUF_SIZE 256
#define FORK_SUCCESS 0
#define FORK_ERR -1
#define RVAL_ERR -1
#define OPTS_END -1

#define DEFAULT_CONFIG_PATH "/etc/conkyscreensaver.conf"
#define USR_CONF_PATH_FORMAT "/home/%s/.conkyscreensaver.conf"

const char WRONG_ARG_ERR[] = "Error while parsing the arguments";

typedef enum { WSTILL_WAIT, WTIME_IS_OUT, WIS_UNLOCKED } waitRVal_t;
typedef struct {
    unsigned long onIdleTimeout;
    timespec_t onIdleRefreshRate;

    unsigned long onLockedIdleTimeout;
    timespec_t onLockedRefreshRate;

    timespec_t onBlankedRefreshRate;
    char display[BUF_SIZE];
} daemonConfigs_t;

void initDaemon(int argc, char *argv[]);
pid_t startScreensaver();
void xscreensaverCommand(char *cmd);
uint8_t waitIdle(unsigned long timeout, const timespec_t *refreshRate,
                 int opts);
void waitXScreensaverUnblanked(const timespec_t *refreshRate);
pid_t runScrLocker();
uint8_t waitUnlocked(pid_t lockerPid, const timespec_t *refreshRate,
                     unsigned long timeout);
void termhdl(int);

pid_t appendBackground();
FORCE_INLINE bool_t idleTimeIsOut(unsigned long timeout);
void showUsage();

void (*currState)();
void onIdle();
void onBlanked();
void onLocked();

const static daemonConfigs_t DEFAULTS = {
    .onIdleTimeout = MIN2MSEC(1.5 / 60),
    .onLockedIdleTimeout = MIN2MSEC(2. / 60),
    .onIdleRefreshRate = {.tv_sec = 0, .tv_nsec = SEC2NANOSEC(0.2)},
    .onBlankedRefreshRate = {.tv_sec = 0, .tv_nsec = SEC2NANOSEC(0.2)},
    .onLockedRefreshRate = {.tv_sec = 0, .tv_nsec = SEC2NANOSEC(0.2)},
    .display = "\":0.0\""};

static XScreenSaverInfo *info;
static Display *display;
static daemonConfigs_t *configs;
bool_t isActive;
static pid_t bgPid;
static pid_t xscrPid;

void parseArgs(int argc, char **argv);
void parseConfFile();
void tryReadConfFile(config_t* config, const char* confPath);


int main(int argc, char *argv[])
{
    initDaemon(argc, argv);
    xscrPid = startScreensaver();
    DPUTS("Daemon is ready");

    currState = onIdle;
    isActive = true;
    while (isActive) {
        (*currState)();
    }

    free(configs);
    kill(xscrPid, SIGKILL);
    XFree(info);
    XCloseDisplay(display);

    return EXIT_SUCCESS;
}

void initDaemon(int argc, char *argv[])
{
    char setDispCmd[BUF_SIZE];

    system("killall xscreensaver &>/dev/null");
    configs = malloc(sizeof(daemonConfigs_t));
    memcpy(configs, &DEFAULTS, sizeof(daemonConfigs_t));

    parseConfFile();
    parseArgs(argc, argv);
    snprintf(setDispCmd, sizeof(setDispCmd), "export DISPLAY=%s",
             configs->display);
    system(setDispCmd);

    if ((display = XOpenDisplay(0)) == NULL)
        abortWithNotif("XOpenDisplay");
    if ((info = XScreenSaverAllocInfo()) == NULL) {
        XCloseDisplay(display);
        abortWithNotif("XScreenSaverAllocInfo");
    }
}

void parseConfFile()
{
    config_t fconfigs;

    char* username = getenv("USER");
    int pathLen = strlen(username) + (strlen(USR_CONF_PATH_FORMAT) - strlen("%s")) + 1;
    char* userConfPath = (char *) malloc(pathLen * sizeof(char));
    snprintf(userConfPath, pathLen, USR_CONF_PATH_FORMAT, username);

    config_init(&fconfigs);
    tryReadConfFile(&fconfigs, userConfPath);

    exit(0);
}

void tryReadConfFile(config_t* config, const char* confPath)
{
    if(! config_read_file(config, confPath))
    {
      fprintf(stderr, "Conf error at %s, %d: %s\n",
              config_error_file(config), config_error_line(config), config_error_text(config));
      exit(EXIT_FAILURE);
    }
}

void parseArgs(int argc, char **argv)
{
    char opt;
    int optIndex = 0;

    static struct option long_options[] = {
        {"help",                  no_argument,        0,  'h'},
        {"idle-timeout",          required_argument,  0,  'i'},
        {"locked-idle-timeout",   required_argument,  0,  'c'},
        {"locked-timeout",        required_argument,  0,  't'},
        {"idle-refresh-rate",     required_argument,  0,  'r'},
        {"locked-refresh-rate",   required_argument,  0,  'l'},
        {"blanked-refresh-rate",  required_argument,  0,  'b'},
        {0, 0, 0, 0}};

    DPUTS("starting to parse args")
    while (true) {
        opt = getopt_long(argc, argv, "i:c:t:r:l:b:h", long_options, &optIndex);
        if (opt == -1) break;

        switch (opt) {
        case 'i':
            if(parseULong(optarg, &configs->onIdleTimeout)) abortWithNotif(WRONG_ARG_ERR);
            break;
        case 'c':
            if(parseULong(optarg, &configs->onLockedIdleTimeout)) abortWithNotif(WRONG_ARG_ERR);
            break;
        case 't':
            break;
        case 'r':
            break;
        case 'l':
            break;
        case 'b':
            break;
        case 'h':
            showUsage();
            return;
        case '?':
            exit(EXIT_FAILURE);
        }
    }
}

void onIdle()
{
    DPUTS("Start again");
    waitIdle(configs->onIdleTimeout, &configs->onIdleRefreshRate, 0);
    bgPid = appendBackground();
    DPRINTF("Specified idle time (%lu ms) is reached.\n",
            configs->onIdleTimeout);
    currState = &onBlanked;
}

void onBlanked()
{
    xscreensaverCommand("-activate");
    waitXScreensaverUnblanked(&configs->onBlankedRefreshRate);
    DPUTS("\n\nExiting XScreensaver");
    currState = &onLocked;
}

void onLocked()
{
    uint8_t waitResult;
    pid_t lockerPid = runScrLocker();

    waitResult = waitUnlocked(lockerPid, &configs->onLockedRefreshRate,
                              configs->onLockedIdleTimeout);
    if (waitResult == WIS_UNLOCKED) {
        kill(bgPid, SIGKILL);
        currState = onIdle;
    } else {
        kill(lockerPid, SIGKILL);
        currState = onBlanked;
    }
}

uint8_t waitUnlocked(pid_t lockerPid, const timespec_t *refreshRate,
                     unsigned long timeout)
{
    while (true) {
        nanosleep(refreshRate, NULL);
        if (idleTimeIsOut(timeout))
            return WTIME_IS_OUT;
        if (waitpid(lockerPid, NULL, WNOHANG) != 0)
            return WIS_UNLOCKED;
    }

    return 0;
}

pid_t appendBackground()
{
    GtkWidget *window;
    GdkColor color = (GdkColor){.red = 0x0, .green = 0x0, .blue = 0x0};
    pid_t cpid = fork();

    if (cpid == FORK_SUCCESS) {
        gtk_init(NULL, NULL);
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_widget_realize(window);
        gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &color);
        gtk_window_fullscreen((GtkWindow *)window);
        gtk_widget_show(window);
        gtk_main();
    } else if (cpid == FORK_ERR)
        abortWithNotif("fork");

    return cpid;
}

void termhdl(int notused)
{
    isActive = false;
}

pid_t startScreensaver()
{
    pid_t cpid = fork();

    if (cpid == FORK_SUCCESS) {
        char *cargv[] = {"/usr/bin/xscreensaver", "-no-splash", NULL};
        if (execvp(cargv[0], &cargv[0]) == RVAL_ERR)
            abortWithNotif("exec");
    } else if (cpid == FORK_ERR)
        abortWithNotif("fork");

    return cpid;
}

uint8_t waitIdle(unsigned long timeout, const timespec_t *refreshRate, int opts)
{
    bool_t repeat = true;

    if (opts & WNOHANG) {
        repeat = false;
    }
    while (repeat) {
        if (idleTimeIsOut(timeout) == true)
            return WTIME_IS_OUT;
        nanosleep(refreshRate, NULL);
    }

    return WSTILL_WAIT;
}

FORCE_INLINE bool_t idleTimeIsOut(unsigned long timeout)
{
    XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
    return info->idle >= timeout ? true : false;
}

pid_t runScrLocker()
{
    pid_t cpid = fork();
    if (cpid == FORK_SUCCESS) {
        execl("/usr/bin/slimlock", "/usr/bin/slimlock", NULL);
    } else if (cpid == FORK_ERR)
        abortWithNotif("fork");

    return cpid;
}

void waitXScreensaverUnblanked(const timespec_t *refreshRate)
{
    char cmdOutp[BUF_SIZE];
    char cmd[BUF_SIZE];
    FILE *pipe;
    uint8_t repeat = true;

    snprintf(cmd, sizeof(cmd),
             "[[ \"`xscreensaver-command -time`\" == *\"non-blanked\"* ]]"
             "&& echo -n %hu || echo -n %hu",
             true, false);

    while (repeat) {
        nanosleep(refreshRate, NULL);

        if ((pipe = popen(cmd, "r")) == NULL)
            abortWithNotif("popen");
        system(cmd);
        fgets(cmdOutp, sizeof(cmdOutp), pipe);
        if (CH2INT(cmdOutp[0]) == true)
            repeat = false;
        pclose(pipe);
    }
}

void xscreensaverCommand(char *cmd)
{
    char *const cargv[3] = {"/usr/bin/xscreensaver-command", cmd, NULL};

    pid_t cpid = fork();
    if (cpid == FORK_SUCCESS) {
        if (execvp(cargv[0], &cargv[0]) == RVAL_ERR)
            abortWithNotif("exec");
    } else if (cpid == FORK_ERR)
        abortWithNotif("fork");

    waitpid(cpid, NULL, 0);
}

void showUsage()
{
    printf("USAGE: \n");
}
