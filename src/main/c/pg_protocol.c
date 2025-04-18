/**
 * pg_protocol.c
 * PostgreSQL Protocol Implementation
 * 
 * This file contains implementations for handling the PostgreSQL wire protocol.
 */

#include "pg_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

/**
 * Read a message from a client
 * 
 * @param client_fd The client socket file descriptor
 * @param buffer Buffer to store the message
 * @param buffer_size Size of the buffer
 * @return Length of the message read, or -1 on error
 */
int pg_read_message(int client_fd, char *buffer, int buffer_size) {
    char type;
    int32_t length;
    int bytes_read;
    
    // Read the message type
    bytes_read = read(client_fd, &type, 1);
    if (bytes_read <= 0) {
        return -1;
    }
    
    // Store the message type in the buffer
    buffer[0] = type;
    
    // Read the message length
    bytes_read = read(client_fd, &length, 4);
    if (bytes_read <= 0) {
        return -1;
    }
    
    // Convert from network byte order
    length = ntohl(length);
    
    // Check if the message fits in the buffer
    if (length > buffer_size - 1) {
        return -1;
    }
    
    // Read the message content
    bytes_read = read(client_fd, buffer + 5, length - 4);
    if (bytes_read != length - 4) {
        return -1;
    }
    
    // Store the length in the buffer
    *((int32_t *)(buffer + 1)) = htonl(length);
    
    return length + 1;
}

/**
 * Send a message to a client
 * 
 * @param client_fd The client socket file descriptor
 * @param type Message type
 * @param buffer Message content
 * @param length Length of the message content
 * @return 0 on success, -1 on error
 */
int pg_send_message(int client_fd, char type, const char *buffer, int length) {
    char *message;
    int32_t total_length;
    int result;
    
    // Calculate the total message length
    total_length = length + 4;
    
    // Allocate memory for the message
    message = malloc(total_length + 1);
    if (!message) {
        return -1;
    }
    
    // Set the message type
    message[0] = type;
    
    // Set the message length
    *((int32_t *)(message + 1)) = htonl(total_length);
    
    // Copy the message content
    if (buffer && length > 0) {
        memcpy(message + 5, buffer, length);
    }
    
    // Send the message
    result = write(client_fd, message, total_length + 1);
    
    // Free the message buffer
    free(message);
    
    return (result == total_length + 1) ? 0 : -1;
}

/**
 * Send an error response to a client
 * 
 * @param client_fd The client socket file descriptor
 * @param code Error code
 * @param message Error message
 * @return 0 on success, -1 on error
 */
int pg_send_error(int client_fd, const char *code, const char *message) {
    char buffer[1024];
    int pos = 0;
    
    // Severity field
    buffer[pos++] = PG_ERR_SEVERITY;
    strcpy(buffer + pos, "ERROR");
    pos += strlen("ERROR") + 1;
    
    // Code field
    buffer[pos++] = PG_ERR_CODE;
    strcpy(buffer + pos, code);
    pos += strlen(code) + 1;
    
    // Message field
    buffer[pos++] = PG_ERR_MESSAGE;
    strcpy(buffer + pos, message);
    pos += strlen(message) + 1;
    
    // Terminator
    buffer[pos++] = 0;
    
    return pg_send_message(client_fd, PG_MSG_ERROR_RESPONSE, buffer, pos);
}

/**
 * Send a notice response to a client
 * 
 * @param client_fd The client socket file descriptor
 * @param message Notice message
 * @return 0 on success, -1 on error
 */
int pg_send_notice(int client_fd, const char *message) {
    char buffer[1024];
    int pos = 0;
    
    // Severity field
    buffer[pos++] = PG_ERR_SEVERITY;
    strcpy(buffer + pos, "NOTICE");
    pos += strlen("NOTICE") + 1;
    
    // Message field
    buffer[pos++] = PG_ERR_MESSAGE;
    strcpy(buffer + pos, message);
    pos += strlen(message) + 1;
    
    // Terminator
    buffer[pos++] = 0;
    
    return pg_send_message(client_fd, PG_MSG_NOTICE_RESPONSE, buffer, pos);
}

/**
 * Send an authentication request to a client
 * 
 * @param client_fd The client socket file descriptor
 * @param auth_type Authentication type
 * @return 0 on success, -1 on error
 */
int pg_send_auth_request(int client_fd, int auth_type) {
    int32_t type = htonl(auth_type);
    return pg_send_message(client_fd, PG_MSG_AUTHENTICATION, (char *)&type, sizeof(type));
}

/**
 * Send an authentication OK message to a client
 * 
 * @param client_fd The client socket file descriptor
 * @return 0 on success, -1 on error
 */
int pg_send_auth_ok(int client_fd) {
    return pg_send_auth_request(client_fd, PG_AUTH_OK);
}

/**
 * Send a ready for query message to a client
 * 
 * @param client_fd The client socket file descriptor
 * @param status Transaction status
 * @return 0 on success, -1 on error
 */
int pg_send_ready_for_query(int client_fd, char status) {
    return pg_send_message(client_fd, PG_MSG_READY_FOR_QUERY, &status, 1);
}

/**
 * Send a row description message to a client
 * 
 * @param client_fd The client socket file descriptor
 * @param num_fields Number of fields
 * @param field_names Array of field names
 * @param field_types Array of field types
 * @return 0 on success, -1 on error
 */
int pg_send_row_description(int client_fd, int num_fields, const char **field_names, int *field_types) {
    char buffer[4096];
    int pos = 0;
    int16_t num_fields_n = htons(num_fields);
    
    // Number of fields
    memcpy(buffer, &num_fields_n, 2);
    pos += 2;
    
    // Field descriptions
    for (int i = 0; i < num_fields; i++) {
        // Field name
        strcpy(buffer + pos, field_names[i]);
        pos += strlen(field_names[i]) + 1;
        
        // Table OID (0 for now)
        *((int32_t *)(buffer + pos)) = 0;
        pos += 4;
        
        // Column attribute number (0 for now)
        *((int16_t *)(buffer + pos)) = 0;
        pos += 2;
        
        // Data type OID
        *((int32_t *)(buffer + pos)) = htonl(field_types[i]);
        pos += 4;
        
        // Data type size (0 for now)
        *((int16_t *)(buffer + pos)) = 0;
        pos += 2;
        
        // Type modifier (0 for now)
        *((int32_t *)(buffer + pos)) = 0;
        pos += 4;
        
        // Format code (0 = text, 1 = binary)
        *((int16_t *)(buffer + pos)) = 0;
        pos += 2;
    }
    
    return pg_send_message(client_fd, PG_MSG_ROW_DESCRIPTION, buffer, pos);
}

/**
 * Send a data row message to a client
 * 
 * @param client_fd The client socket file descriptor
 * @param num_fields Number of fields
 * @param values Array of field values
 * @param lengths Array of field lengths
 * @return 0 on success, -1 on error
 */
int pg_send_data_row(int client_fd, int num_fields, const char **values, int *lengths) {
    char buffer[4096];
    int pos = 0;
    int16_t num_fields_n = htons(num_fields);
    
    // Number of fields
    memcpy(buffer, &num_fields_n, 2);
    pos += 2;
    
    // Field values
    for (int i = 0; i < num_fields; i++) {
        if (values[i] == NULL) {
            // NULL value
            *((int32_t *)(buffer + pos)) = htonl(-1);
            pos += 4;
        } else {
            // Value length
            *((int32_t *)(buffer + pos)) = htonl(lengths[i]);
            pos += 4;
            
            // Value data
            memcpy(buffer + pos, values[i], lengths[i]);
            pos += lengths[i];
        }
    }
    
    return pg_send_message(client_fd, PG_MSG_DATA_ROW, buffer, pos);
}

/**
 * Send a command complete message to a client
 * 
 * @param client_fd The client socket file descriptor
 * @param tag Command tag
 * @return 0 on success, -1 on error
 */
int pg_send_command_complete(int client_fd, const char *tag) {
    char buffer[64];
    strcpy(buffer, tag);
    return pg_send_message(client_fd, PG_MSG_COMMAND_COMPLETE, buffer, strlen(buffer) + 1);
}

/**
 * Send a parameter status message to a client
 * 
 * @param client_fd The client socket file descriptor
 * @param name Parameter name
 * @param value Parameter value
 * @return 0 on success, -1 on error
 */
int pg_send_parameter_status(int client_fd, const char *name, const char *value) {
    char buffer[1024];
    int name_len = strlen(name) + 1;
    int value_len = strlen(value) + 1;
    
    strcpy(buffer, name);
    strcpy(buffer + name_len, value);
    
    return pg_send_message(client_fd, PG_MSG_PARAMETER_STATUS, buffer, name_len + value_len);
}

/**
 * Send a backend key data message to a client
 * 
 * @param client_fd The client socket file descriptor
 * @param pid Process ID
 * @param key Secret key
 * @return 0 on success, -1 on error
 */
int pg_send_backend_key_data(int client_fd, int32_t pid, int32_t key) {
    char buffer[8];
    
    *((int32_t *)buffer) = htonl(pid);
    *((int32_t *)(buffer + 4)) = htonl(key);
    
    return pg_send_message(client_fd, PG_MSG_BACKEND_KEY_DATA, buffer, 8);
}
