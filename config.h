#ifndef CONFIG_H
#define CONFIG_H

struct user_entry {
    char role[16];
    char username[32];
    char password[64];
};

int load_config(const char *filename);
int check_credentials(const char *role, const char *user, const char *pass);

#endif
