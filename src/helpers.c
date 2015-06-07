#include "helpers.h"

uint8_t parseULong(const char * from, U64* to)
{
    char * endptr;
    *to = strtoul(from, &endptr, 10);
    return *endptr == '\0' ? 0 : -1;
}

uint8_t parseLong(const char * from, long* to)
{
    char * endptr;
    *to = strtoul(from, &endptr, 10);
    return *endptr == '\0' ? 0 : -1;
}

uint8_t parseTimeT(const char * from, time_t* to)
{
    char * endptr;
    *to = strtoul(from, &endptr, 10);
    return *endptr == '\0' ? 0 : -1;
}

void abortem(const char * msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void abortWithNotif(const char * msg)
{
    static char notifyIcon[] = "dialog-error";
    notify_init(msg);
    NotifyNotification * notif = notify_notification_new(msg, strerror(errno), notifyIcon);
    notify_notification_show(notif, NULL);
    g_object_unref(G_OBJECT(notif));
    notify_uninit();
    puts("Aborted");

    exit(EXIT_FAILURE);
}

void free2(void** ptr, U64 size)
{
    U64 i;

    for (i = 0; i < size; ++i) {
        free(ptr[i]);
    }
    free(ptr);
}

char* getUserConfPath()
{
    char* username = getenv("USER");
    int pathLen = strlen(username) + (strlen(USR_CONF_PATH_FORMAT) - strlen("%s")) + 1;
    char* userConfPath = (char *) malloc(pathLen * sizeof(char));
    snprintf(userConfPath, pathLen, USR_CONF_PATH_FORMAT, username);
    return userConfPath;
}

void showUsage()
{
    printf("USAGE: \n");
}
