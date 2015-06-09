
#define _DEFAULT_SOURCE
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include "main_defines.h"
#include "helpers.h"

#include "parse_helpers.h"
#include "int_types.h"
#include "conf_parse.h"
#include "default_configs.h"

static XScreenSaverInfo *info;
static Display *display;
static daemonConfigs_t *configs;
bool_t isActive;
static pid_t bgPid;
static pid_t xscrPid;

void initDaemon(int argc, char *argv[]);
pid_t startScreensaver();
void xscreensaverCommand(char *cmd);
uint8_t waitIdle(U64 timeout, const timespec_t *refreshRate, int opts);
void waitXScreensaverUnblanked(const timespec_t *refreshRate);
pid_t runScrLocker();
uint8_t waitUnlocked(pid_t lockerPid, const timespec_t *refreshRate, U64 timeout);
pid_t appendBackground();
FORCE_INLINE bool_t idleTimeIsOut(U64 timeout);

void (*currState)();
void onIdle();
void onBlanked();
void onLocked();

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
    memset(configs, 0, sizeof(daemonConfigs_t));

    parseCmdLineArgs(argc, argv, configs);
    parseConfFile(configs);
    printf("%f\n", (double)configs->onLockedIdleTimeout);
    setDefaultConfigs(configs);
    printf("%f\n", (double)configs->onLockedIdleTimeout);

    snprintf(setDispCmd, sizeof(setDispCmd), "export DISPLAY=%s", configs->display);
    system(setDispCmd);

    if ((display = XOpenDisplay(0)) == NULL)
        abortWithNotif("XOpenDisplay");
    if ((info = XScreenSaverAllocInfo()) == NULL) {
        XCloseDisplay(display);
        abortWithNotif("XScreenSaverAllocInfo");
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

    waitResult = waitUnlocked(lockerPid, &configs->onLockedRefreshRate, configs->onLockedIdleTimeout);
    if (waitResult == WIS_UNLOCKED) {
        kill(bgPid, SIGKILL);
        currState = onIdle;
    } else {
        kill(lockerPid, SIGKILL);
        currState = onBlanked;
    }
}

uint8_t waitUnlocked(pid_t lockerPid, const timespec_t *refreshRate, U64 timeout)
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

uint8_t waitIdle(U64 timeout, const timespec_t *refreshRate, int opts)
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

FORCE_INLINE bool_t idleTimeIsOut(U64 timeout)
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
    int i;
    puts("USAGE:");
    for (i = 0; i < SIZE(cmdline_options) - 1; ++i) {
        printf(" -%c  --%s \n", cmdline_options[i].val, cmdline_options[i].name);
    }

    exit(EXIT_FAILURE);
}

