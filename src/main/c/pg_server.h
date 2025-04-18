/**
 * pg_server.h
 * PostgreSQL Protocol Server Emulator
 * 
 * This file contains declarations for the PostgreSQL protocol server emulator.
 */

#ifndef PG_SERVER_H
#define PG_SERVER_H

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct PGServer PGServer;
typedef struct PGClientConn PGClientConn;

/* Server configuration */
typedef struct {
    const char *host;        /* Host to bind to */
    int port;                /* Port to listen on */
    const char *data_dir;    /* Data directory */
    const char *log_file;    /* Log file path */
    int max_connections;     /* Maximum number of connections */
    bool ssl_enabled;        /* Whether SSL is enabled */
    const char *ssl_cert;    /* SSL certificate path */
    const char *ssl_key;     /* SSL key path */
    bool verbose;            /* Enable verbose logging */
} PGServerConfig;

/* Client connection state */
struct PGClientConn {
    int fd;                  /* Client socket file descriptor */
    char *user;              /* Authenticated user */
    char *database;          /* Connected database */
    bool authenticated;      /* Whether client is authenticated */
    char txn_status;         /* Transaction status (I, T, E) */
    int32_t backend_pid;     /* Backend process ID */
    int32_t secret_key;      /* Secret key for cancel requests */
    void *ssl;               /* SSL connection (if enabled) */
    void *user_data;         /* User-defined data */
    PGServer *server;        /* Reference to the server */
};

/* Message callback function types */
typedef int (*PGStartupCallback)(PGClientConn *client, const char *buffer, int length);
typedef int (*PGQueryCallback)(PGClientConn *client, const char *query);
typedef int (*PGPasswordCallback)(PGClientConn *client, const char *password);
typedef int (*PGTerminateCallback)(PGClientConn *client);
typedef int (*PGSyncCallback)(PGClientConn *client);
typedef int (*PGDescribeCallback)(PGClientConn *client, char describe_type, const char *name);
typedef int (*PGBindCallback)(PGClientConn *client, const char *buffer, int length);
typedef int (*PGExecuteCallback)(PGClientConn *client, const char *portal, int max_rows);
typedef int (*PGParseCallback)(PGClientConn *client, const char *stmt_name, const char *query, int num_params);
typedef int (*PGCancelCallback)(PGClientConn *client, int32_t pid, int32_t key);
typedef int (*PGSSLRequestCallback)(PGClientConn *client);
typedef int (*PGUnknownCallback)(PGClientConn *client, char type, const char *buffer, int length);

/* Callback structure */
typedef struct {
    PGStartupCallback startup;
    PGQueryCallback query;
    PGPasswordCallback password;
    PGTerminateCallback terminate;
    PGSyncCallback sync;
    PGDescribeCallback describe;
    PGBindCallback bind;
    PGExecuteCallback execute;
    PGParseCallback parse;
    PGCancelCallback cancel;
    PGSSLRequestCallback ssl_request;
    PGUnknownCallback unknown;
} PGCallbacks;

/* Server context */
struct PGServer {
    PGServerConfig config;   /* Server configuration */
    int server_fd;           /* Server socket file descriptor */
    PGClientConn **clients;  /* Array of client connections */
    int num_clients;         /* Number of active clients */
    bool running;            /* Whether server is running */
    void *user_data;         /* User-defined data */
    PGCallbacks callbacks;   /* Message callbacks */
};

/* Function declarations */
PGServer *pg_server_create(const PGServerConfig *config);
int pg_server_start(PGServer *server);
int pg_server_run(PGServer *server);
int pg_server_stop(PGServer *server);
void pg_server_destroy(PGServer *server);

int pg_server_handle_client(PGServer *server, PGClientConn *client);
int pg_server_add_client(PGServer *server, int client_fd);
int pg_server_remove_client(PGServer *server, PGClientConn *client);

/* Set callback functions */
void pg_server_set_callbacks(PGServer *server, const PGCallbacks *callbacks);
void pg_server_set_startup_callback(PGServer *server, PGStartupCallback callback);
void pg_server_set_query_callback(PGServer *server, PGQueryCallback callback);
void pg_server_set_password_callback(PGServer *server, PGPasswordCallback callback);
void pg_server_set_terminate_callback(PGServer *server, PGTerminateCallback callback);
void pg_server_set_sync_callback(PGServer *server, PGSyncCallback callback);
void pg_server_set_describe_callback(PGServer *server, PGDescribeCallback callback);
void pg_server_set_bind_callback(PGServer *server, PGBindCallback callback);
void pg_server_set_execute_callback(PGServer *server, PGExecuteCallback callback);
void pg_server_set_parse_callback(PGServer *server, PGParseCallback callback);
void pg_server_set_cancel_callback(PGServer *server, PGCancelCallback callback);
void pg_server_set_ssl_request_callback(PGServer *server, PGSSLRequestCallback callback);
void pg_server_set_unknown_callback(PGServer *server, PGUnknownCallback callback);

/* Default callback implementations */
int pg_default_startup_callback(PGClientConn *client, const char *buffer, int length);
int pg_default_query_callback(PGClientConn *client, const char *query);
int pg_default_password_callback(PGClientConn *client, const char *password);
int pg_default_terminate_callback(PGClientConn *client);
int pg_default_sync_callback(PGClientConn *client);
int pg_default_describe_callback(PGClientConn *client, char describe_type, const char *name);
int pg_default_bind_callback(PGClientConn *client, const char *buffer, int length);
int pg_default_execute_callback(PGClientConn *client, const char *portal, int max_rows);
int pg_default_parse_callback(PGClientConn *client, const char *stmt_name, const char *query, int num_params);
int pg_default_cancel_callback(PGClientConn *client, int32_t pid, int32_t key);
int pg_default_ssl_request_callback(PGClientConn *client);
int pg_default_unknown_callback(PGClientConn *client, char type, const char *buffer, int length);

/* Helper functions */
int pg_server_send_startup_messages(PGClientConn *client);

#endif /* PG_SERVER_H */
