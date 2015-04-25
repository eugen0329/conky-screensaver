#include "helpers.h"


uint8_t parseULong(const char * from, unsigned long* to)
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


