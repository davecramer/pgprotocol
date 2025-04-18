/**
 * pg_query.c
 * PostgreSQL Query Handler
 * 
 * This file contains implementations for handling PostgreSQL queries.
 */

#include "pg_server.h"
#include "pg_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Query types */
typedef enum {
    QUERY_SELECT,
    QUERY_INSERT,
    QUERY_UPDATE,
    QUERY_DELETE,
    QUERY_BEGIN,
    QUERY_COMMIT,
    QUERY_ROLLBACK,
    QUERY_CREATE,
    QUERY_DROP,
    QUERY_ALTER,
    QUERY_UNKNOWN
} QueryType;

/* Forward declarations */
static QueryType pg_get_query_type(const char *query);
static int pg_handle_select(PGClientConn *client, const char *query);
static int pg_handle_insert(PGClientConn *client, const char *query);
static int pg_handle_update(PGClientConn *client, const char *query);
static int pg_handle_delete(PGClientConn *client, const char *query);
static int pg_handle_transaction(PGClientConn *client, const char *query, QueryType type);

/**
 * Default query callback
 * 
 * @param client Client connection
 * @param query Query string
 * @return 0 on success, -1 on error
 */
int pg_default_query_callback(PGClientConn *client, const char *query) {
    QueryType type;
    
    // Get query type
    type = pg_get_query_type(query);
    
    // Handle query based on type
    switch (type) {
        case QUERY_SELECT:
            return pg_handle_select(client, query);
        
        case QUERY_INSERT:
            return pg_handle_insert(client, query);
        
        case QUERY_UPDATE:
            return pg_handle_update(client, query);
        
        case QUERY_DELETE:
            return pg_handle_delete(client, query);
        
        case QUERY_BEGIN:
        case QUERY_COMMIT:
        case QUERY_ROLLBACK:
            return pg_handle_transaction(client, query, type);
        
        default:
            // Send error for unsupported query types
            pg_send_error(client->fd, "42601", "Unsupported query type");
            pg_send_ready_for_query(client->fd, client->txn_status);
            return -1;
    }
}

/**
 * Get the type of a query
 * 
 * @param query Query string
 * @return Query type
 */
static QueryType pg_get_query_type(const char *query) {
    char first_word[16] = {0};
    int i = 0;
    
    // Skip leading whitespace
    while (isspace(*query)) {
        query++;
    }
    
    // Get first word
    while (*query && !isspace(*query) && i < 15) {
        first_word[i++] = toupper(*query++);
    }
    first_word[i] = '\0';
    
    // Determine query type based on first word
    if (strcmp(first_word, "SELECT") == 0) {
        return QUERY_SELECT;
    } else if (strcmp(first_word, "INSERT") == 0) {
        return QUERY_INSERT;
    } else if (strcmp(first_word, "UPDATE") == 0) {
        return QUERY_UPDATE;
    } else if (strcmp(first_word, "DELETE") == 0) {
        return QUERY_DELETE;
    } else if (strcmp(first_word, "BEGIN") == 0) {
        return QUERY_BEGIN;
    } else if (strcmp(first_word, "COMMIT") == 0) {
        return QUERY_COMMIT;
    } else if (strcmp(first_word, "ROLLBACK") == 0) {
        return QUERY_ROLLBACK;
    } else if (strcmp(first_word, "CREATE") == 0) {
        return QUERY_CREATE;
    } else if (strcmp(first_word, "DROP") == 0) {
        return QUERY_DROP;
    } else if (strcmp(first_word, "ALTER") == 0) {
        return QUERY_ALTER;
    }
    
    return QUERY_UNKNOWN;
}

/**
 * Handle a SELECT query
 * 
 * @param client Client connection
 * @param query Query string
 * @return 0 on success, -1 on error
 */
static int pg_handle_select(PGClientConn *client, const char *query) {
    // For demonstration, we'll just return a simple result set
    const char *field_names[] = {"id", "name", "value"};
    int field_types[] = {23, 25, 25}; // int4, text, text
    
    // Send row description
    pg_send_row_description(client->fd, 3, field_names, field_types);
    
    // Send data rows
    const char *values1[] = {"1", "Row 1", "Value 1"};
    int lengths1[] = {1, 5, 7};
    pg_send_data_row(client->fd, 3, values1, lengths1);
    
    const char *values2[] = {"2", "Row 2", "Value 2"};
    int lengths2[] = {1, 5, 7};
    pg_send_data_row(client->fd, 3, values2, lengths2);
    
    // Send command complete
    pg_send_command_complete(client->fd, "SELECT 2");
    
    // Send ready for query
    pg_send_ready_for_query(client->fd, client->txn_status);
    
    return 0;
}

/**
 * Handle an INSERT query
 * 
 * @param client Client connection
 * @param query Query string
 * @return 0 on success, -1 on error
 */
static int pg_handle_insert(PGClientConn *client, const char *query) {
    // For demonstration, we'll just return a success message
    pg_send_command_complete(client->fd, "INSERT 0 1");
    pg_send_ready_for_query(client->fd, client->txn_status);
    
    return 0;
}

/**
 * Handle an UPDATE query
 * 
 * @param client Client connection
 * @param query Query string
 * @return 0 on success, -1 on error
 */
static int pg_handle_update(PGClientConn *client, const char *query) {
    // For demonstration, we'll just return a success message
    pg_send_command_complete(client->fd, "UPDATE 1");
    pg_send_ready_for_query(client->fd, client->txn_status);
    
    return 0;
}

/**
 * Handle a DELETE query
 * 
 * @param client Client connection
 * @param query Query string
 * @return 0 on success, -1 on error
 */
static int pg_handle_delete(PGClientConn *client, const char *query) {
    // For demonstration, we'll just return a success message
    pg_send_command_complete(client->fd, "DELETE 1");
    pg_send_ready_for_query(client->fd, client->txn_status);
    
    return 0;
}

/**
 * Handle a transaction query (BEGIN, COMMIT, ROLLBACK)
 * 
 * @param client Client connection
 * @param query Query string
 * @param type Query type
 * @return 0 on success, -1 on error
 */
static int pg_handle_transaction(PGClientConn *client, const char *query, QueryType type) {
    switch (type) {
        case QUERY_BEGIN:
            client->txn_status = PG_TXN_TRANSACTION;
            pg_send_command_complete(client->fd, "BEGIN");
            break;
        
        case QUERY_COMMIT:
            client->txn_status = PG_TXN_IDLE;
            pg_send_command_complete(client->fd, "COMMIT");
            break;
        
        case QUERY_ROLLBACK:
            client->txn_status = PG_TXN_IDLE;
            pg_send_command_complete(client->fd, "ROLLBACK");
            break;
        
        default:
            return -1;
    }
    
    pg_send_ready_for_query(client->fd, client->txn_status);
    
    return 0;
}
