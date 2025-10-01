#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#define MAX_USERS 50
static struct user_entry users[MAX_USERS];
static int user_count = 0;

int load_config(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(len+1);
    fread(data, 1, len, f);
    data[len] = 0;
    fclose(f);

    cJSON *root = cJSON_Parse(data);
    if (!root) { free(data); return -1; }

    cJSON *arr = cJSON_GetObjectItem(root, "users");
    user_count = 0;
    cJSON *u;
    cJSON_ArrayForEach(u, arr) {
        if (user_count >= MAX_USERS) break;
        strcpy(users[user_count].role, cJSON_GetObjectItem(u,"role")->valuestring);
        strcpy(users[user_count].username, cJSON_GetObjectItem(u,"username")->valuestring);
        strcpy(users[user_count].password, cJSON_GetObjectItem(u,"password")->valuestring);
        user_count++;
    }

    cJSON_Delete(root);
    free(data);
    return 0;
}

int check_credentials(const char *role, const char *user, const char *pass) {
    for (int i=0; i<user_count; i++) {
        if (strcasecmp(users[i].role, role)==0 &&
            strcmp(users[i].username, user)==0 &&
            strcmp(users[i].password, pass)==0) {
            return 1;
        }
    }
    return 0;
}
