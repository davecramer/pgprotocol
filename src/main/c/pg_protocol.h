/**
 * pg_protocol.h
 * PostgreSQL Protocol Definitions
 * 
 * This file contains definitions for the PostgreSQL wire protocol
 * as specified in the PostgreSQL documentation.
 */

#ifndef PG_PROTOCOL_H
#define PG_PROTOCOL_H

#include <stdint.h>

/* PostgreSQL protocol version */
#define PG_PROTOCOL_MAJOR 3
#define PG_PROTOCOL_MINOR 0

/* Message types (first byte of message) */
/* Frontend message types */
#define PG_MSG_STARTUP       0
#define PG_MSG_QUERY         'Q'
#define PG_MSG_TERMINATE     'X'
#define PG_MSG_PASSWORD      'p'
#define PG_MSG_SYNC          'S'
#define PG_MSG_DESCRIBE      'D'
#define PG_MSG_BIND          'B'
#define PG_MSG_EXECUTE       'E'
#define PG_MSG_PARSE         'P'
#define PG_MSG_CANCEL        0
#define PG_MSG_SSL_REQUEST   0

/* Backend message types */
#define PG_MSG_AUTHENTICATION 'R'
#define PG_MSG_ERROR_RESPONSE 'E'
#define PG_MSG_NOTICE_RESPONSE 'N'
#define PG_MSG_READY_FOR_QUERY 'Z'
#define PG_MSG_ROW_DESCRIPTION 'T'
#define PG_MSG_DATA_ROW        'D'
#define PG_MSG_COMMAND_COMPLETE 'C'
#define PG_MSG_PARAMETER_STATUS 'S'
#define PG_MSG_BACKEND_KEY_DATA 'K'
#define PG_MSG_EMPTY_QUERY_RESPONSE 'I'

/* Authentication types */
#define PG_AUTH_OK               0
#define PG_AUTH_KERBEROS_V5      2
#define PG_AUTH_CLEARTEXT        3
#define PG_AUTH_MD5              5
#define PG_AUTH_SCM_CREDENTIAL   6
#define PG_AUTH_GSS              7
#define PG_AUTH_GSS_CONTINUE     8
#define PG_AUTH_SSPI             9

/* Transaction status indicators */
#define PG_TXN_IDLE              'I'
#define PG_TXN_TRANSACTION       'T'
#define PG_TXN_FAILED            'E'

/* Error and notice field codes */
#define PG_ERR_SEVERITY          'S'
#define PG_ERR_CODE              'C'
#define PG_ERR_MESSAGE           'M'
#define PG_ERR_DETAIL            'D'
#define PG_ERR_HINT              'H'
#define PG_ERR_POSITION          'P'
#define PG_ERR_INTERNAL_POSITION 'p'
#define PG_ERR_INTERNAL_QUERY    'q'
#define PG_ERR_WHERE             'W'
#define PG_ERR_SCHEMA_NAME       's'
#define PG_ERR_TABLE_NAME        't'
#define PG_ERR_COLUMN_NAME       'c'
#define PG_ERR_DATA_TYPE_NAME    'd'
#define PG_ERR_CONSTRAINT_NAME   'n'
#define PG_ERR_FILE              'F'
#define PG_ERR_LINE              'L'
#define PG_ERR_ROUTINE           'R'

/* Protocol message structures */
typedef struct {
    int32_t length;      /* Length of message contents in bytes, including self */
    int32_t protocol;    /* Protocol version number (3.0) */
} PGStartupMessage;

typedef struct {
    char type;           /* Message type */
    int32_t length;      /* Length of message contents in bytes, including self */
} PGMessageHeader;

/* Function declarations */
int pg_read_message(int client_fd, char *buffer, int buffer_size);
int pg_send_message(int client_fd, char type, const char *buffer, int length);
int pg_send_error(int client_fd, const char *code, const char *message);
int pg_send_notice(int client_fd, const char *message);
int pg_send_auth_request(int client_fd, int auth_type);
int pg_send_auth_ok(int client_fd);
int pg_send_ready_for_query(int client_fd, char status);
int pg_send_row_description(int client_fd, int num_fields, const char **field_names, int *field_types);
int pg_send_data_row(int client_fd, int num_fields, const char **values, int *lengths);
int pg_send_command_complete(int client_fd, const char *tag);
int pg_send_parameter_status(int client_fd, const char *name, const char *value);
int pg_send_backend_key_data(int client_fd, int32_t pid, int32_t key);

#endif /* PG_PROTOCOL_H */
