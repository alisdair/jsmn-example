/* log: stderr logging information. */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "log.h"

void log_die(char *msg, ...)
{
    va_list argp;

    log_null(msg);

    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);

    fprintf(stderr, "\n");
    abort();
}

void log_info(char *msg, ...)
{
    va_list argp;

    log_null(msg);

    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);

    fprintf(stderr, "\n");
}
