#include "helpers.h"

void abortem(const char * msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}
