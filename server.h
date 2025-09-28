#ifndef SERVER_H
#define SERVER_H

#include "client_handler.h"

void start_server(int port);
void remove_client(struct client_data *client);
void add_client(struct client_data *client);
void *telemetry_thread(void *arg);

extern float speed_offset;
extern int direction_state;
extern int battery_level;      
extern int temp_c; 

#endif

