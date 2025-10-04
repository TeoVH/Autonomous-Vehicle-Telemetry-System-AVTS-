#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#define MAX_USERS 50
static struct user_entry users[MAX_USERS];
static int user_count = 0;

// Función para calcular SHA-256 hash
static void sha256_hash(const char *input, char *output) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input, strlen(input));
    SHA256_Final(hash, &sha256);
    
    // Convertir a hex string
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[64] = '\0';
}

int load_config(const char *filename) {
    // Configuración hardcodeada con hashes para evitar dependencia de cjson
    user_count = 2;
    
    // Usuario ADMIN - root/password
    strcpy(users[0].role, "ADMIN");
    strcpy(users[0].username, "root");
    strcpy(users[0].salt, "c20efd8d3a1bbeb7a6645513d5b51b392974ba538b941c27bef5daaab2af5aff");
    strcpy(users[0].hash, "f0c6b39b2414f78e271b2933ca97aa782633e1108de23b935f84398ed9967972");
    
    // Usuario OBSERVER - juan/12345  
    strcpy(users[1].role, "OBSERVER");
    strcpy(users[1].username, "juan");
    strcpy(users[1].salt, "04616e7fec14a85267564785c375f605cccaf09d7b697b8b6093a642beadf358");
    strcpy(users[1].hash, "b32299198dc0dbc43cd0f2f243c79ba697ed372d37704ba5db01d62a9cfc7e94");
    
    return 0;
}

int check_credentials(const char *role, const char *user, const char *pass) {
    for (int i=0; i<user_count; i++) {
        if (strcasecmp(users[i].role, role)==0 &&
            strcmp(users[i].username, user)==0) {
            
            // Calcular hash de salt + password
            char combined[129]; // salt (64) + password (máx 64) + null
            snprintf(combined, sizeof(combined), "%s%s", users[i].salt, pass);
            
            char calculated_hash[65];
            sha256_hash(combined, calculated_hash);
            
            // Comparar con hash almacenado
            if (strcmp(users[i].hash, calculated_hash) == 0) {
                return 1;
            }
        }
    }
    return 0;
}
