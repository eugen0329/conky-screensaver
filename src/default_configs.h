#ifndef DEFAULT_CONFIGS_H_8YZGLCXB
#define DEFAULT_CONFIGS_H_8YZGLCXB

#include "main_defines.h"

const daemonConfigs_t DEFAULTS = {
    .onIdleTimeout = MIN2MSEC(20.5 / 60),
    .onLockedIdleTimeout = MIN2MSEC(2. / 60),
    .onIdleRefreshRate = {.tv_sec = 1, .tv_nsec = SEC2NANOSEC(0.2)},
    .onBlankedRefreshRate = {.tv_sec = 0, .tv_nsec = SEC2NANOSEC(0.2)},
    .onLockedRefreshRate = {.tv_sec = 0, .tv_nsec = SEC2NANOSEC(0.2)},
    .display = "\":0.0\""};

#endif /* end of include guard: DEFAULT_CONFIGS_H_8YZGLCXB */
