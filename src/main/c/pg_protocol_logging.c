/**
 * pg_protocol_logging.c
 * Logging wrappers for PostgreSQL protocol functions
 */

#include "pg_server.h"
#include "pg_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "/usr/local/pgsql/18/include/server/libpq/protocol.h"

// Logging wrapper for startup callback
int pg_logging_startup_callback(PGClientConn *client, const char *buffer, int length) {
    pg_log_info("Protocol: Startup message received from client %d", client->fd);
    
    // Extract protocol version
    int32_t protocol_version;
    memcpy(&protocol_version, buffer + 4, 4);
    protocol_version = ntohl(protocol_version);
    pg_log_debug("Protocol: Version %d.%d", protocol_version / 65536, protocol_version % 65536);
    
    // Log parameters
    const char *p = buffer + 8; // Skip length and protocol version
    pg_log_debug("Protocol: Startup parameters:");
    while (p < buffer + length) {
        const char *param = p;
        p += strlen(p) + 1;
        if (p >= buffer + length) break;
        
        const char *value = p;
        p += strlen(p) + 1;
        
        if (*param == 0) break;
        pg_log_debug("Protocol:   %s = %s", param, value);
    }
    
    // Call the original callback
    int result = pg_default_startup_callback(client, buffer, length);
    
    if (result < 0) {
        pg_log_error("Protocol: Startup message handling failed");
    } else {
        pg_log_info("Protocol: Startup message handled successfully");
    }
    
    return result;
}

// Logging wrapper for query callback
int pg_logging_query_callback(PGClientConn *client, const char *query) {
    pg_log_info("Protocol: Query message (PqMsg_Query) received from client %d", client->fd);
    pg_log_debug("Protocol: SQL: %s", query);
    
    // Call the original callback
    int result = pg_default_query_callback(client, query);
    
    if (result < 0) {
        pg_log_error("Protocol: Query handling failed");
    } else {
        pg_log_info("Protocol: Query handled successfully");
    }
    
    return result;
}

// Logging wrapper for password callback
int pg_logging_password_callback(PGClientConn *client, const char *password) {
    pg_log_info("Protocol: Password message (PqMsg_PasswordMessage) received from client %d", client->fd);
    pg_log_debug("Protocol: Password authentication attempt");
    
    // Call the original callback
    int result = pg_default_password_callback(client, password);
    
    if (result < 0) {
        pg_log_error("Protocol: Password authentication failed");
    } else {
        pg_log_info("Protocol: Password authentication successful");
    }
    
    return result;
}

// Logging wrapper for terminate callback
int pg_logging_terminate_callback(PGClientConn *client) {
    pg_log_info("Protocol: Terminate message (PqMsg_Terminate) received from client %d", client->fd);
    
    // Call the original callback
    int result = pg_default_terminate_callback(client);
    
    if (result < 0) {
        pg_log_error("Protocol: Terminate handling failed");
    } else {
        pg_log_info("Protocol: Terminate handled successfully");
    }
    
    return result;
}

// Logging wrapper for sync callback
int pg_logging_sync_callback(PGClientConn *client) {
    pg_log_info("Protocol: Sync message (PqMsg_Sync) received from client %d", client->fd);
    
    // Call the original callback
    int result = pg_default_sync_callback(client);
    
    if (result < 0) {
        pg_log_error("Protocol: Sync handling failed");
    } else {
        pg_log_debug("Protocol: Sync handled successfully, sent ReadyForQuery");
    }
    
    return result;
}

// Logging wrapper for describe callback
int pg_logging_describe_callback(PGClientConn *client, char describe_type, const char *name) {
    pg_log_info("Protocol: Describe message (PqMsg_Describe) received from client %d", client->fd);
    pg_log_debug("Protocol: Describe type: %c, name: %s", 
                describe_type, name ? name : "(empty)");
    
    // Call the original callback
    int result = pg_default_describe_callback(client, describe_type, name);
    
    if (result < 0) {
        pg_log_error("Protocol: Describe handling failed");
    } else {
        pg_log_debug("Protocol: Describe handled successfully");
    }
    
    return result;
}

// Logging wrapper for bind callback
int pg_logging_bind_callback(PGClientConn *client, const char *buffer, int length) {
    pg_log_info("Protocol: Bind message (PqMsg_Bind) received from client %d", client->fd);
    
    // Extract portal and statement names
    const char *portal = buffer;
    const char *statement = buffer + strlen(portal) + 1;
    
    pg_log_debug("Protocol: Bind portal: %s, statement: %s", 
                portal[0] ? portal : "(unnamed)", 
                statement[0] ? statement : "(unnamed)");
    
    // Call the original callback
    int result = pg_default_bind_callback(client, buffer, length);
    
    if (result < 0) {
        pg_log_error("Protocol: Bind handling failed");
    } else {
        pg_log_debug("Protocol: Bind handled successfully");
    }
    
    return result;
}

// Logging wrapper for execute callback
int pg_logging_execute_callback(PGClientConn *client, const char *portal, int max_rows) {
    pg_log_info("Protocol: Execute message (PqMsg_Execute) received from client %d", client->fd);
    pg_log_debug("Protocol: Execute portal: %s, max rows: %d", 
                portal[0] ? portal : "(unnamed)", max_rows);
    
    // Call the original callback
    int result = pg_default_execute_callback(client, portal, max_rows);
    
    if (result < 0) {
        pg_log_error("Protocol: Execute handling failed");
    } else {
        pg_log_debug("Protocol: Execute handled successfully");
    }
    
    return result;
}

// Logging wrapper for parse callback
int pg_logging_parse_callback(PGClientConn *client, const char *stmt_name, const char *query, int num_params) {
    pg_log_info("Protocol: Parse message (PqMsg_Parse) received from client %d", client->fd);
    pg_log_debug("Protocol: Parse statement: %s, query: %s, params: %d", 
                stmt_name && stmt_name[0] ? stmt_name : "(unnamed)", 
                query, num_params);
    
    // Call the original callback
    int result = pg_default_parse_callback(client, stmt_name, query, num_params);
    
    if (result < 0) {
        pg_log_error("Protocol: Parse handling failed");
    } else {
        pg_log_debug("Protocol: Parse handled successfully");
    }
    
    return result;
}

// Logging wrapper for cancel callback
int pg_logging_cancel_callback(PGClientConn *client, int32_t pid, int32_t key) {
    pg_log_info("Protocol: Cancel request received for backend PID %d", pid);
    pg_log_debug("Protocol: Cancel request PID: %d, key: %d", pid, key);
    
    // Call the original callback
    int result = pg_default_cancel_callback(client,pid, key);
    
    if (result < 0) {
        pg_log_error("Protocol: Cancel handling failed");
    } else {
        pg_log_debug("Protocol: Cancel handled successfully");
    }
    
    return result;
}

// Logging wrapper for SSL request callback
int pg_logging_ssl_request_callback(PGClientConn *client) {
    pg_log_info("Protocol: SSL request received from client %d", client->fd);
    
    // Call the original callback
    int result = pg_default_ssl_request_callback(client);
    
    if (result < 0) {
        pg_log_error("Protocol: SSL request handling failed");
    } else {
        pg_log_debug("Protocol: SSL request handled successfully");
    }
    
    return result;
}

// Logging wrapper for unknown callback
int pg_logging_unknown_callback(PGClientConn *client, char msg_type, const char *buffer, int length) {
    pg_log_warning("Protocol: Unknown message type '%c' (0x%02x) received from client %d", 
                 msg_type ? msg_type : '?', (unsigned char)msg_type, client->fd);
    pg_log_debug("Protocol: Unknown message length: %d bytes", length);
    
    // Dump first few bytes in hex for debugging
    if (length > 0) {
        char hex_dump[100] = {0};
        char *p = hex_dump;
        int bytes_to_dump = length < 16 ? length : 16;
        
        for (int i = 0; i < bytes_to_dump; i++) {
            p += sprintf(p, "%02x ", (unsigned char)buffer[i]);
        }
        
        pg_log_debug("Protocol: Message hex dump (first %d bytes): %s", bytes_to_dump, hex_dump);
    }
    
    // Call the original callback
    int result = pg_default_unknown_callback(client, msg_type, buffer, length);
    
    if (result < 0) {
        pg_log_error("Protocol: Unknown message handling failed");
    } else {
        pg_log_debug("Protocol: Unknown message handled successfully");
    }
    
    return result;
}

// Function to set up all logging callbacks
void pg_server_set_logging_callbacks(PGServer *server) {
    pg_log_info("Setting up protocol logging callbacks");
    
    server->callbacks.startup = pg_logging_startup_callback;
    server->callbacks.query = pg_logging_query_callback;
    server->callbacks.password = pg_logging_password_callback;
    server->callbacks.terminate = pg_logging_terminate_callback;
    server->callbacks.sync = pg_logging_sync_callback;
    server->callbacks.describe = pg_logging_describe_callback;
    server->callbacks.bind = pg_logging_bind_callback;
    server->callbacks.execute = pg_logging_execute_callback;
    server->callbacks.parse = pg_logging_parse_callback;
    server->callbacks.cancel = pg_logging_cancel_callback;
    server->callbacks.ssl_request = pg_logging_ssl_request_callback;
    server->callbacks.unknown = pg_logging_unknown_callback;
}

// Helper function to log outgoing messages
void pg_log_outgoing_message(PGClientConn *client, char msg_type, int length) {
    const char *msg_name = "Unknown";
    
    switch (msg_type) {
        case PqMsg_AuthenticationRequest: msg_name = "Authentication"; break;
        case PqMsg_BackendKeyData: msg_name = "BackendKeyData"; break;
        case PqMsg_ParameterStatus: msg_name = "ParameterStatus"; break;
        case PqMsg_ReadyForQuery: msg_name = "ReadyForQuery"; break;
        case PqMsg_RowDescription: msg_name = "RowDescription"; break;
        case PqMsg_DataRow: msg_name = "DataRow"; break;
        case PqMsg_CommandComplete: msg_name = "CommandComplete"; break;
        case PqMsg_ErrorResponse: msg_name = "ErrorResponse"; break;
        case PqMsg_NoticeResponse: msg_name = "NoticeResponse"; break;
        case PqMsg_EmptyQueryResponse: msg_name = "EmptyQueryResponse"; break;
        case PqMsg_ParseComplete: msg_name = "ParseComplete"; break;
        case PqMsg_BindComplete: msg_name = "BindComplete"; break;
        case PqMsg_CloseComplete: msg_name = "CloseComplete"; break;
        case PqMsg_NoData: msg_name = "NoData"; break;
        case PqMsg_NotificationResponse: msg_name = "NotificationResponse"; break;
        case PqMsg_ParameterDescription: msg_name = "ParameterDescription"; break;
        case PqMsg_CopyInResponse: msg_name = "CopyInResponse"; break;
        case PqMsg_CopyOutResponse: msg_name = "CopyOutResponse"; break;
        case PqMsg_CopyBothResponse: msg_name = "CopyBothResponse"; break;
        case PqMsg_CopyData: msg_name = "CopyData"; break;
        case PqMsg_CopyDone: msg_name = "CopyDone"; break;
        case PqMsg_CopyFail: msg_name = "CopyFail"; break;
        case PqMsg_FunctionCallResponse: msg_name = "FunctionCallResponse"; break;
        case PqMsg_NegotiateProtocolVersion: msg_name = "NegotiateProtocolVersion"; break;
    }
    
    pg_log_debug("Protocol: Sending %s message to client %d (%d bytes)", 
                msg_name, client->fd, length);
}

// Enhanced send function with logging
int pg_send_with_logging(PGClientConn *client, const void *buffer, size_t length) {
    if (length >= 5) {
        char msg_type = ((const char *)buffer)[0];
        pg_log_outgoing_message(client, msg_type, length);
    }
    
    int result = send(client->fd, buffer, length, 0);
    
    if (result < 0) {
        pg_log_error("Protocol: Failed to send message to client %d: %s", 
                   client->fd, strerror(errno));
    }
    
    return result;
}
