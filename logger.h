#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

void init_logger(const char *filename);
void close_logger();
void log_event(const char *fmt, ...);
void log_user_action(const char *username, const char *role, const char *action, const char *details);
void log_connection_event(const char *event_type, const char *username, const char *role, const char *ip, int port);

#endif

