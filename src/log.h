#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void logD(const char *fmt, ...);
void die(const char *fmt, ...);

#endif
