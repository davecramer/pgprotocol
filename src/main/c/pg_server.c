/**
 * pg_server.c
 * PostgreSQL Protocol Server Emulator Implementation
 */

 #include "pg_server.h"
 #include "pg_log.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <errno.h>
 #include <fcntl.h>
 #include <sys/select.h>
 #include "/usr/local/pgsql/18/include/server/libpq/protocol.h"
 
 #define BUFFER_SIZE 8192
 
 // Create server instance
 PGServer *pg_server_create(const PGServerConfig *config) {
     PGServer *server = (PGServer *)malloc(sizeof(PGServer));
     if (!server) return NULL;
 
     // Copy configuration
     memcpy(&server->config, config, sizeof(PGServerConfig));
     
     // Initialize server state
     server->server_fd = -1;
     server->clients = (PGClientConn **)calloc(config->max_connections, sizeof(PGClientConn *));
     server->num_clients = 0;
     server->running = false;
     server->user_data = NULL;
 
     // Set default callbacks
     server->callbacks.startup = pg_default_startup_callback;
     server->callbacks.query = pg_default_query_callback;
     server->callbacks.password = pg_default_password_callback;
     server->callbacks.terminate = pg_default_terminate_callback;
     server->callbacks.sync = pg_default_sync_callback;
     server->callbacks.describe = pg_default_describe_callback;
     server->callbacks.bind = pg_default_bind_callback;
     server->callbacks.execute = pg_default_execute_callback;
     server->callbacks.parse = pg_default_parse_callback;
     server->callbacks.cancel = pg_default_cancel_callback;
     server->callbacks.ssl_request = pg_default_ssl_request_callback;
     server->callbacks.unknown = pg_default_unknown_callback;
 
     return server;
 }
 
 // Start the server
 int pg_server_start(PGServer *server) {
     struct sockaddr_in addr;
     int opt = 1;
 
     // Create socket
     server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
     if (server->server_fd < 0) {
         return -1;
     }
 
     // Set socket options
     setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
 
     // Configure address
     addr.sin_family = AF_INET;
     addr.sin_addr.s_addr = INADDR_ANY;
     addr.sin_port = htons(server->config.port);
 
     // Bind
     if (bind(server->server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
         close(server->server_fd);
         return -1;
     }
 
     // Listen
     if (listen(server->server_fd, server->config.max_connections) < 0) {
         close(server->server_fd);
         return -1;
     }
 
     server->running = true;
     return 0;
 }
 
 // Main server loop
 int pg_server_run(PGServer *server) {
     fd_set read_fds;
     int max_fd;
     struct timeval tv;
 
     while (server->running) {
         FD_ZERO(&read_fds);
         FD_SET(server->server_fd, &read_fds);
         max_fd = server->server_fd;
 
         // Add client sockets to fd_set
         for (int i = 0; i < server->config.max_connections; i++) {
             if (server->clients[i]) {
                 FD_SET(server->clients[i]->fd, &read_fds);
                 if (server->clients[i]->fd > max_fd) {
                     max_fd = server->clients[i]->fd;
                 }
             }
         }
 
         tv.tv_sec = 1;
         tv.tv_usec = 0;
 
         int activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
         if (activity < 0) continue;
 
         // New connection
         if (FD_ISSET(server->server_fd, &read_fds)) {
             struct sockaddr_in client_addr;
             socklen_t addr_len = sizeof(client_addr);
             int client_fd = accept(server->server_fd, (struct sockaddr *)&client_addr, &addr_len);
             
             if (client_fd >= 0) {
                 pg_server_add_client(server, client_fd);
             }
         }
 
         // Handle client messages
         for (int i = 0; i < server->config.max_connections; i++) {
             PGClientConn *client = server->clients[i];
             if (client && FD_ISSET(client->fd, &read_fds)) {
                 if (pg_server_handle_client(server, client) < 0) {
                     pg_server_remove_client(server, client);
                 }
             }
         }
     }
 
     return 0;
 }
 
 // Handle client messages
 int pg_server_handle_client(PGServer *server, PGClientConn *client) {
     char buffer[BUFFER_SIZE];
     int bytes_read;
 
     bytes_read = recv(client->fd, buffer, sizeof(buffer), 0);
     if (bytes_read <= 0) {
         return -1;
     }
 
     // First byte is message type
     char msg_type = buffer[0];
     int32_t length;
     memcpy(&length, buffer + 1, 4);
     length = ntohl(length);
 
     // Handle different message types
     switch (msg_type) {
         case 0: // Startup message
             return server->callbacks.startup(client, buffer, bytes_read);
         
         case PqMsg_Query: // Simple Query
             return server->callbacks.query(client, buffer + 5);
         
         case PqMsg_Parse: // Parse
             return server->callbacks.parse(client, buffer + 5, NULL, 0);
         
         case PqMsg_Bind: // Bind
             return server->callbacks.bind(client, buffer + 5, length - 4);
         
         case PqMsg_Execute: // Execute
             return server->callbacks.execute(client, buffer + 5, 0);
         
         case PqMsg_Describe: // Describe
             return server->callbacks.describe(client, buffer[5], buffer + 6);
         
         case PqMsg_Sync: // Sync
             return server->callbacks.sync(client);
         
         case PqMsg_Terminate: // Terminate
             return server->callbacks.terminate(client);
         
         default:
             return server->callbacks.unknown(client, msg_type, buffer + 5, length - 4);
     }
 }
 
 // Add new client connection
 int pg_server_add_client(PGServer *server, int client_fd) {
     if (server->num_clients >= server->config.max_connections) {
         close(client_fd);
         return -1;
     }
 
     PGClientConn *client = (PGClientConn *)malloc(sizeof(PGClientConn));
     if (!client) {
         close(client_fd);
         return -1;
     }
 
     // Initialize client connection
     client->fd = client_fd;
     client->user = NULL;
     client->database = NULL;
     client->authenticated = false;
     client->txn_status = 'I';
     client->backend_pid = getpid() + client_fd;
     client->secret_key = rand();
     client->ssl = NULL;
     client->user_data = NULL;
     client->server = server;
 
     // Find empty slot
     for (int i = 0; i < server->config.max_connections; i++) {
         if (!server->clients[i]) {
             server->clients[i] = client;
             server->num_clients++;
             return 0;
             // Continuing from where we left off...

            return 0;
        }
    }

    free(client);
    close(client_fd);
    return -1;
}

// Remove client connection
int pg_server_remove_client(PGServer *server, PGClientConn *client) {
    for (int i = 0; i < server->config.max_connections; i++) {
        if (server->clients[i] == client) {
            close(client->fd);
            free(client->user);
            free(client->database);
            free(client);
            server->clients[i] = NULL;
            server->num_clients--;
            return 0;
        }
    }
    return -1;
}

// Stop server
int pg_server_stop(PGServer *server) {
    server->running = false;
    
    // Close all client connections
    for (int i = 0; i < server->config.max_connections; i++) {
        if (server->clients[i]) {
            pg_server_remove_client(server, server->clients[i]);
        }
    }

    // Close server socket
    if (server->server_fd >= 0) {
        close(server->server_fd);
        server->server_fd = -1;
    }

    return 0;
}

// Destroy server
void pg_server_destroy(PGServer *server) {
    if (server) {
        pg_server_stop(server);
        free(server->clients);
        free(server);
    }
}

// Helper function to send startup messages
int pg_server_send_startup_messages(PGClientConn *client) {
    // Send AuthenticationOk
    char auth_ok[9] = {PqMsg_AuthenticationRequest, 0, 0, 0, 8, 0, 0, 0, 0};
    if (send(client->fd, auth_ok, 9, 0) < 0) return -1;

    // Send ParameterStatus messages
    const char *params[][2] = {
        {"server_version", "14.0"},
        {"client_encoding", "UTF8"},
        {"server_encoding", "UTF8"},
        {"DateStyle", "ISO, MDY"}
    };

    for (int i = 0; i < 4; i++) {
        int len = 4 + 1 + strlen(params[i][0]) + 1 + strlen(params[i][1]) + 1;
        char *msg = malloc(len);
        char *p = msg;
        
        *p++ = PqMsg_ParameterStatus;
        *(int32_t *)p = htonl(len - 1); p += 4;
        strcpy(p, params[i][0]); p += strlen(params[i][0]) + 1;
        strcpy(p, params[i][1]);
        
        if (send(client->fd, msg, len, 0) < 0) {
            free(msg);
            return -1;
        }
        free(msg);
    }

    // Send BackendKeyData
    char key_data[13] = {PqMsg_BackendKeyData, 0, 0, 0, 12};
    *(int32_t *)(key_data + 5) = htonl(client->backend_pid);
    *(int32_t *)(key_data + 9) = htonl(client->secret_key);
    if (send(client->fd, key_data, 13, 0) < 0) return -1;

    // Send ReadyForQuery
    char ready[6] = {PqMsg_ReadyForQuery, 0, 0, 0, 5, 'I'};
    if (send(client->fd, ready, 6, 0) < 0) return -1;

    return 0;
}

// Default callback implementations
int pg_default_startup_callback(PGClientConn *client, const char *buffer, int length) {
    // Parse startup message and extract parameters
    const char *p = buffer + 8; // Skip length and protocol version
    while (p < buffer + length) {
        const char *param = p;
        p += strlen(p) + 1;
        const char *value = p;
        p += strlen(p) + 1;
        
        if (*param == 0) break;

        if (strcmp(param, "user") == 0) {
            client->user = strdup(value);
        } else if (strcmp(param, "database") == 0) {
            client->database = strdup(value);
        }
    }

    return pg_server_send_startup_messages(client);
}

// Implement other default callbacks
int pg_default_query_callback(PGClientConn *client, const char *query) {
    // Send EmptyQueryResponse or RowDescription + DataRow(s) + CommandComplete
    char empty_response[6] = {PqMsg_EmptyQueryResponse, 0, 0, 0, 4};
    return send(client->fd, empty_response, 5, 0);
}


// Callback setters
void pg_server_set_callbacks(PGServer *server, const PGCallbacks *callbacks) {
    memcpy(&server->callbacks, callbacks, sizeof(PGCallbacks));
}

void pg_server_set_startup_callback(PGServer *server, PGStartupCallback callback) {
    server->callbacks.startup = callback ? callback : pg_default_startup_callback;
}

void pg_server_set_query_callback(PGServer *server, PGQueryCallback callback) {
    server->callbacks.query = callback ? callback : pg_default_query_callback;
}

void pg_server_set_password_callback(PGServer *server, PGPasswordCallback callback) {
    server->callbacks.password = callback ? callback : pg_default_password_callback;
}

void pg_server_set_terminate_callback(PGServer *server, PGTerminateCallback callback) {
    server->callbacks.terminate = callback ? callback : pg_default_terminate_callback;
}

void pg_server_set_sync_callback(PGServer *server, PGSyncCallback callback) {
    server->callbacks.sync = callback ? callback : pg_default_sync_callback;
}

void pg_server_set_describe_callback(PGServer *server, PGDescribeCallback callback) {
    server->callbacks.describe = callback ? callback : pg_default_describe_callback;
}

void pg_server_set_bind_callback(PGServer *server, PGBindCallback callback) {
    server->callbacks.bind = callback ? callback : pg_default_bind_callback;
}

void pg_server_set_execute_callback(PGServer *server, PGExecuteCallback callback) {
    server->callbacks.execute = callback ? callback : pg_default_execute_callback;
}

void pg_server_set_parse_callback(PGServer *server, PGParseCallback callback) {
    server->callbacks.parse = callback ? callback : pg_default_parse_callback;
}

void pg_server_set_cancel_callback(PGServer *server, PGCancelCallback callback) {
    server->callbacks.cancel = callback ? callback : pg_default_cancel_callback;
}

void pg_server_set_ssl_request_callback(PGServer *server, PGSSLRequestCallback callback) {
    server->callbacks.ssl_request = callback ? callback : pg_default_ssl_request_callback;
}

void pg_server_set_unknown_callback(PGServer *server, PGUnknownCallback callback) {
    server->callbacks.unknown = callback ? callback : pg_default_unknown_callback;
}

int pg_default_password_callback(PGClientConn *client, const char *password) {
    // Send AuthenticationOk by default
    char auth_ok[9] = {PqMsg_AuthenticationRequest, 0, 0, 0, 8, 0, 0, 0, AUTH_REQ_OK};
    return send(client->fd, auth_ok, 9, 0);
}

int pg_default_terminate_callback(PGClientConn *client) {
    // Simply return success, client will be cleaned up by the server
    return 0;
}

int pg_default_sync_callback(PGClientConn *client) {
    // Send ReadyForQuery with idle status
    char ready[6] = {PqMsg_ReadyForQuery, 0, 0, 0, 5, 'I'};
    return send(client->fd, ready, 6, 0);
}

int pg_default_describe_callback(PGClientConn *client, char describe_type, const char *name) {
    // Send NoData response
    char no_data[5] = {PqMsg_NoData, 0, 0, 0, 4};
    return send(client->fd, no_data, 5, 0);
}

int pg_default_bind_callback(PGClientConn *client, const char *data, int length) {
    // Send BindComplete
    char bind_complete[5] = {PqMsg_BindComplete, 0, 0, 0, 4};
    return send(client->fd, bind_complete, 5, 0);
}

int pg_default_execute_callback(PGClientConn *client, const char *portal, int max_rows) {
    // Send EmptyQueryResponse
    char empty_response[5] = {PqMsg_EmptyQueryResponse, 0, 0, 0, 4};
    if (send(client->fd, empty_response, 5, 0) < 0) return -1;

    // Send CommandComplete with empty tag
    char command_complete[6] = {PqMsg_CommandComplete, 0, 0, 0, 5, 0};
    return send(client->fd, command_complete, 6, 0);
}

int pg_default_parse_callback(PGClientConn *client, const char *query, const char *stmt_name, int num_params) {
    // Send ParseComplete
    char parse_complete[5] = {PqMsg_ParseComplete, 0, 0, 0, 4};
    return send(client->fd, parse_complete, 5, 0);
}

int pg_default_cancel_callback(PGClientConn *client, int32_t pid, int32_t key) {
    // By default, just acknowledge the cancel request
    return 0;
}

int pg_default_ssl_request_callback(PGClientConn *client) {
    // By default, reject SSL
    char reject_ssl = 'N';
    return send(client->fd, &reject_ssl, 1, 0);
}

int pg_default_unknown_callback(PGClientConn *client, char msg_type, const char *data, int length) {
    // Send ErrorResponse for unknown message types
    char error_msg[] = {
        PqMsg_ErrorResponse,       // Error message type
        0, 0, 0, 85,              // Length
        'S', 'E', 'R', 'R', 'O', 'R', 0,           // Severity
        'C', '4', '2', '6', '0', '1', 0,           // Code
        'M', 'U', 'n', 'k', 'n', 'o', 'w', 'n', ' ',
        'm', 'e', 's', 's', 'a', 'g', 'e', ' ',
        't', 'y', 'p', 'e', 0,                      // Message
        0                          // Terminator
    };
    
    if (send(client->fd, error_msg, sizeof(error_msg), 0) < 0) return -1;

    // Send ReadyForQuery
    char ready[6] = {PqMsg_ReadyForQuery, 0, 0, 0, 5, 'I'};
    return send(client->fd, ready, 6, 0);
}