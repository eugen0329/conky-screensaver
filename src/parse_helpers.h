#ifndef PARSE_HELPERS_H_RVS6KFP9
#define PARSE_HELPERS_H_RVS6KFP9

#include <string.h>
#include <stdlib.h>
#include "inline.h"


FORCE_INLINE unsigned long getTokensCount(const char* str, const char* separators)
{
    unsigned long len = strlen(str), nSeparators = strlen(separators);
    unsigned long i, j, count = 0;

    for (i = 0; i < len; ++i) {
        for (j = 0; j < nSeparators; ++j) {
            if(str[i] == separators[j]) {
                count ++;
                break;
            }
        }
    }
    return count + 1;
}


char** parseTokens(const char* str, const char* separators, unsigned long* count)
{
    char* strDup = (char *) calloc(strlen(str) + 1, sizeof(char));
    const char* token;
    char** tokens;
    int i = 0;

    strcpy(strDup, str);
    *count = getTokensCount(str, separators);
    tokens = (char **) malloc((*count) * sizeof(char *));

    token = strtok(strDup, separators);
    while(token != NULL) {
        tokens[i] = (char *) malloc( (strlen(token) + 1) * sizeof(char));
        strcpy(tokens[i], token);
        token = strtok(NULL, separators);
        i++;
    }

    return tokens;
}


#endif /* end of include guard: PARSE_HELPERS_H_RVS6KFP9 */
