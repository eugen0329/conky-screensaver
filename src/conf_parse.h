#ifndef CONF_PARSE_H_VIW539K7
#define CONF_PARSE_H_VIW539K7

#include <libconfig.h>
#include <getopt.h>

#include <time.h>

#include "main_defines.h"

#include "helpers.h"
#include "parse_helpers.h"

#define FRACTIONAL_DIVIDER "."

void tryReadConfFile(config_t* config, const char* confPath)
{
    if(! config_read_file(config, confPath)) {
      fprintf(stderr, "Conf error at %s, %d: %s\n",
              config_error_file(config), config_error_line(config), config_error_text(config));
      exit(EXIT_FAILURE);
    }

}

void parseCmdLineArgs(int argc, char **argv, daemonConfigs_t* conf)
{
    char opt;
    int optIndex = 0;
    char** tokens;
    U64 tokensCount = 0;

    static struct option long_options[] = {
        {"help",                  no_argument,        0,  'h'},
        {"idle-timeout",          required_argument,  0,  'i'},
        {"locked-idle-timeout",   required_argument,  0,  'c'},
        {"locked-timeout",        required_argument,  0,  't'},
        {"idle-refresh-rate",     required_argument,  0,  'r'},
        {"locked-refresh-rate",   required_argument,  0,  'l'},
        {"blanked-refresh-rate",  required_argument,  0,  'b'},
        {0, 0, 0, 0}};

    DPUTS("starting to parse args");
    while (true) {
        opt = getopt_long(argc, argv, "i:c:t:r:l:b:h", long_options, &optIndex);
        if (opt == -1) break;

        switch (opt) {
        case 'i':
            if(parseULong(optarg, &conf->onIdleTimeout)) abortWithNotif(WRONG_ARG_ERR);
            break;
        case 'c':
            if(parseULong(optarg, &conf->onLockedIdleTimeout)) abortWithNotif(WRONG_ARG_ERR);
            break;
        case 't':
            printf("-t is not implemented yet\n");
            break;
        case 'r':
            tokens = parseTokens(optarg, FRACTIONAL_DIVIDER, &tokensCount);
            if(tokensCount != 2) abortWithNotif(WRONG_ARG_ERR);
            if(parseTimeT(tokens[0], &(conf->onIdleRefreshRate).tv_sec)) abortWithNotif(WRONG_ARG_ERR);
            if(parseLong(tokens[1], &(conf->onIdleRefreshRate).tv_nsec)) abortWithNotif(WRONG_ARG_ERR);
            break;
        case 'l':
            tokens = parseTokens(optarg, FRACTIONAL_DIVIDER, &tokensCount);
            if(tokensCount != 2) abortWithNotif(WRONG_ARG_ERR);
            if(parseTimeT(tokens[0], &(conf->onLockedRefreshRate).tv_sec)) abortWithNotif(WRONG_ARG_ERR);
            if(parseLong(tokens[1], &(conf->onLockedRefreshRate).tv_nsec)) abortWithNotif(WRONG_ARG_ERR);
            break;
        case 'b':
            tokens = parseTokens(optarg, FRACTIONAL_DIVIDER, &tokensCount);
            if(tokensCount != 2) abortWithNotif(WRONG_ARG_ERR);
            if(parseTimeT(tokens[0], &(conf->onBlankedRefreshRate).tv_sec)) abortWithNotif(WRONG_ARG_ERR);
            if(parseLong(tokens[1], &(conf->onBlankedRefreshRate).tv_nsec)) abortWithNotif(WRONG_ARG_ERR);
            break;
        case 'h':
            showUsage();
            return;
        case '?':
            exit(EXIT_FAILURE);
        }
    }
}

void parseConfFile(daemonConfigs_t* conf)
{
    config_t config;
    char* userConfPath = getUserConfPath();
    char** tokens;
    U64 tokensCount = 0;

    const char* strBuf;
    double floatBuf;

    config_init(&config);
    tryReadConfFile(&config, userConfPath);

    if(config_lookup_float(&config, "onIdleTimeout", &floatBuf)) {
        DPRINTF( "conf file: parsed onIdleTimeout == %f", floatBuf )
        conf->onIdleTimeout = floatBuf;
    }
    if(config_lookup_float(&config, "onLockedIdleTimeout", &floatBuf)) {
        DPRINTF( "conf file: parsed onLockedIdleTimeout == %f", floatBuf )
        conf->onLockedIdleTimeout = floatBuf;
    }
    if(config_lookup_string(&config, "onIdleRefreshRate", &strBuf)) {
        tokens = parseTokens(strBuf, FRACTIONAL_DIVIDER, &tokensCount);
        conf->onLockedIdleTimeout = floatBuf;
        if(parseTimeT(tokens[0], &(conf->onIdleRefreshRate).tv_sec)) abortWithNotif(WRONG_ARG_ERR);
        if(parseLong(tokens[1], &(conf->onIdleRefreshRate).tv_nsec)) abortWithNotif(WRONG_ARG_ERR);
    }
    if(config_lookup_string(&config, "onLockedRefreshRate", &strBuf)) {
        tokens = parseTokens(optarg, FRACTIONAL_DIVIDER, &tokensCount);
        if(tokensCount != 2) abortWithNotif(WRONG_ARG_ERR);
        if(parseTimeT(tokens[0], &(conf->onLockedRefreshRate).tv_sec)) abortWithNotif(WRONG_ARG_ERR);
        if(parseLong(tokens[1], &(conf->onLockedRefreshRate).tv_nsec)) abortWithNotif(WRONG_ARG_ERR);
    }

    if(config_lookup_string(&config, "onBlankedRefreshRate", &strBuf)) {
        tokens = parseTokens(optarg, FRACTIONAL_DIVIDER, &tokensCount);
        if(tokensCount != 2) abortWithNotif(WRONG_ARG_ERR);
        if(parseTimeT(tokens[0], &(conf->onBlankedRefreshRate).tv_sec)) abortWithNotif(WRONG_ARG_ERR);
        if(parseLong(tokens[1], &(conf->onBlankedRefreshRate).tv_nsec)) abortWithNotif(WRONG_ARG_ERR);
    }
}


void setDefaultConfigs(daemonConfigs_t* conf)
{
    if(conf->onIdleTimeout == (onIdleTimeout_t) 0) {
        conf->onIdleTimeout = DEFAULTS.onIdleTimeout;
    }
    if(conf->onLockedIdleTimeout == (onLockedTimeout_t) 0) {
        conf->onLockedIdleTimeout = DEFAULTS.onLockedIdleTimeout;
    }
    if( memcmp(&conf->onIdleRefreshRate, &NULL_TIMESPEC, sizeof(timespec_t)) == 0 ) {
        conf->onIdleRefreshRate = DEFAULTS.onIdleRefreshRate;
    }
    if( memcmp(&conf->onBlankedRefreshRate, &NULL_TIMESPEC, sizeof(timespec_t)) == 0 ) {
        conf->onBlankedRefreshRate = DEFAULTS.onBlankedRefreshRate;
    }
    if( memcmp(&conf->onLockedRefreshRate, &NULL_TIMESPEC, sizeof(timespec_t)) == 0 ) {
        conf->onLockedRefreshRate = DEFAULTS.onLockedRefreshRate;
    }
}

#endif /* end of include guard: CONF_PARSE_H_VIW539K7 */