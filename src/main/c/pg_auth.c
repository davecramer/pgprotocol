/**
 * pg_auth.c
 * PostgreSQL Authentication Handler
 * 
 * This file contains implementations for handling PostgreSQL authentication.
 */

#include "pg_server.h"
#include "pg_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <openssl/md5.h>

/* Default users */
typedef struct {
    const char *username;
    const char *password;
} PGUser;

static PGUser default_users[] = {
    {"postgres", "postgres"},
    {"test", "test"},
    {NULL, NULL}
};

/**
 * Convert binary data to hexadecimal string
 * 
 * @param data Binary data
 * @param len Data length
 * @param hex Output hexadecimal string (must be at least 2*len+1 bytes)
 */
static void bin_to_hex(unsigned char *data, int len, char *hex) {
    static const char hex_chars[] = "0123456789abcdef";
    int i;
    
    for (i = 0; i < len; i++) {
        hex[i*2] = hex_chars[(data[i] >> 4) & 0xF];
        hex[i*2+1] = hex_chars[data[i] & 0xF];
    }
    
    hex[len*2] = '\0';
}

/**
 * Generate MD5 hash for password authentication
 * 
 * @param password Password
 * @param username Username
 * @param salt Salt
 * @param result Output hash (must be at least 36 bytes)
 */
static void pg_md5_hash(const char *password, const char *username, const char *salt, char *result) {
    MD5_CTX ctx;
    unsigned char md5[MD5_DIGEST_LENGTH];
    char hex[MD5_DIGEST_LENGTH*2+1];
    
    // First hash: password + username
    MD5_Init(&ctx);
    MD5_Update(&ctx, password, strlen(password));
    MD5_Update(&ctx, username, strlen(username));
    MD5_Final(md5, &ctx);
    
    // Convert to hex
    bin_to_hex(md5, MD5_DIGEST_LENGTH, hex);
    
    // Second hash: hex + salt
    MD5_Init(&ctx);
    MD5_Update(&ctx, hex, strlen(hex));
    MD5_Update(&ctx, salt, strlen(salt));
    MD5_Final(md5, &ctx);
    
    // Format result: md5 + hex
    strcpy(result, "md5");
    bin_to_hex(md5, MD5_DIGEST_LENGTH, result + 3);
}

/**
 * Default authentication callback
 * 
 * @param client Client connection
 * @param username Username
 * @param password Password
 * @return 0 on success, -1 on error
 */
int pg_default_auth_callback(PGClientConn *client, const char *username, const char *password) {
    int i;
    
    // Check if username exists
    for (i = 0; default_users[i].username != NULL; i++) {
        if (strcmp(username, default_users[i].username) == 0) {
            // If password is NULL, send MD5 password request
            if (password == NULL) {
                char salt[5];
                int32_t auth_type = htonl(PG_AUTH_MD5);
                
                // Generate random salt
                srand(time(NULL));
                salt[0] = 'a' + (rand() % 26);
                salt[1] = 'a' + (rand() % 26);
                salt[2] = 'a' + (rand() % 26);
                salt[3] = 'a' + (rand() % 26);
                salt[4] = '\0';
                
                // Send MD5 password request
                pg_send_message(client->fd, PG_MSG_AUTHENTICATION, (char *)&auth_type, 4);
                pg_send_message(client->fd, 0, salt, 4);
                
                return 0;
            }
            
            // Check password
            if (strcmp(password, default_users[i].password) == 0) {
                return 0;
            }
            
            return -1;
        }
    }
    
    return -1;
}

/**
 * Handle password message
 * 
 * @param client Client connection
 * @param password Password
 * @return 0 on success, -1 on error
 */
int pg_handle_password(PGClientConn *client, const char *password) {
    // Call authentication callback
    if (pg_default_auth_callback(client, client->user, password) != 0) {
        pg_send_error(client->fd, "28000", "Invalid password");
        return -1;
    }
    
    // Send authentication OK
    pg_send_auth_ok(client->fd);
    
    return 0;
}
