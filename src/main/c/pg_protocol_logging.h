/**
 * pg_protocol_logging.h
 * Logging wrappers for PostgreSQL protocol functions
 */

#ifndef PG_PROTOCOL_LOGGING_H
#define PG_PROTOCOL_LOGGING_H

#include "pg_server.h"

// Logging wrapper functions for protocol callbacks
int pg_logging_startup_callback(PGClientConn *client, const char *buffer, int length);
int pg_logging_query_callback(PGClientConn *client, const char *query);
int pg_logging_password_callback(PGClientConn *client, const char *password);
int pg_logging_terminate_callback(PGClientConn *client);
int pg_logging_sync_callback(PGClientConn *client);
int pg_logging_describe_callback(PGClientConn *client, char describe_type, const char *name);
int pg_logging_bind_callback(PGClientConn *client, const char *buffer, int length);
int pg_logging_execute_callback(PGClientConn *client, const char *portal, int max_rows);
int pg_logging_parse_callback(PGClientConn *client, const char *stmt_name, const char *query, int num_params);
int pg_logging_cancel_callback(PGClientConn *client, int32_t pid, int32_t key);
int pg_logging_ssl_request_callback(PGClientConn *client);
int pg_logging_unknown_callback(PGClientConn *client, char msg_type, const char *buffer, int length);

// Function to set up all logging callbacks
void pg_server_set_logging_callbacks(PGServer *server);

// Helper function to log outgoing messages
void pg_log_outgoing_message(PGClientConn *client, char msg_type, int length);

// Enhanced send function with logging
int pg_send_with_logging(PGClientConn *client, const void *buffer, size_t length);

#endif /* PG_PROTOCOL_LOGGING_H */
