#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

void init_logger(const char *filename);
void close_logger();
void log_event(const char *fmt, ...);

#endif

