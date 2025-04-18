/**
 * pg_log.c
 * Logging functionality for PostgreSQL Protocol Server Emulator
 */

#include "pg_log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Global log configuration */
PGLogConfig pg_log_config = {
    .log_file = NULL,
    .log_level = PG_LOG_INFO,
    .include_timestamp = 1,
    .include_pid = 1
};

/* Log level strings */
static const char *log_level_strings[] = {
    "ERROR",
    "WARNING",
    "INFO",
    "DEBUG"
};

/* Initialize logging */
void pg_log_init(FILE *log_file, PGLogLevel level) {
    pg_log_config.log_file = log_file ? log_file : stderr;
    pg_log_config.log_level = level;
}

/* Set log level */
void pg_log_set_level(PGLogLevel level) {
    pg_log_config.log_level = level;
}

/* Set log file */
void pg_log_set_file(FILE *log_file) {
    if (pg_log_config.log_file != stderr && pg_log_config.log_file != stdout) {
        fclose(pg_log_config.log_file);
    }
    pg_log_config.log_file = log_file ? log_file : stderr;
}

/* Close logging */
void pg_log_close(void) {
    if (pg_log_config.log_file != stderr && pg_log_config.log_file != stdout) {
        fclose(pg_log_config.log_file);
        pg_log_config.log_file = stderr;
    }
}

/* Generic log function */
void pg_log(PGLogLevel level, const char *format, ...) {
    if (level > pg_log_config.log_level) {
        return;
    }

    FILE *out = pg_log_config.log_file ? pg_log_config.log_file : stderr;
    
    /* Print timestamp if enabled */
    if (pg_log_config.include_timestamp) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(out, "[%s] ", timestamp);
    }
    
    /* Print log level */
    fprintf(out, "[%s] ", log_level_strings[level]);
    
    /* Print PID if enabled */
    if (pg_log_config.include_pid) {
        fprintf(out, "[%d] ", getpid());
    }
    
    /* Print the actual message */
    va_list args;
    va_start(args, format);
    vfprintf(out, format, args);
    va_end(args);
    
    /* Add newline if not present */
    if (format[strlen(format) - 1] != '\n') {
        fprintf(out, "\n");
    }
    
    /* Flush output */
    fflush(out);
}

/* Convenience functions for different log levels */
void pg_log_error(const char *format, ...) {
    if (PG_LOG_ERROR > pg_log_config.log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    FILE *out = pg_log_config.log_file ? pg_log_config.log_file : stderr;
    
    /* Print timestamp if enabled */
    if (pg_log_config.include_timestamp) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(out, "[%s] ", timestamp);
    }
    
    /* Print log level */
    fprintf(out, "[%s] ", log_level_strings[PG_LOG_ERROR]);
    
    /* Print PID if enabled */
    if (pg_log_config.include_pid) {
        fprintf(out, "[%d] ", getpid());
    }
    
    /* Print the actual message */
    vfprintf(out, format, args);
    va_end(args);
    
    /* Add newline if not present */
    if (format[strlen(format) - 1] != '\n') {
        fprintf(out, "\n");
    }
    
    /* Flush output */
    fflush(out);
}

void pg_log_warning(const char *format, ...) {
    if (PG_LOG_WARNING > pg_log_config.log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    FILE *out = pg_log_config.log_file ? pg_log_config.log_file : stderr;
    
    /* Print timestamp if enabled */
    if (pg_log_config.include_timestamp) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(out, "[%s] ", timestamp);
    }
    
    /* Print log level */
    fprintf(out, "[%s] ", log_level_strings[PG_LOG_WARNING]);
    
    /* Print PID if enabled */
    if (pg_log_config.include_pid) {
        fprintf(out, "[%d] ", getpid());
    }
    
    /* Print the actual message */
    vfprintf(out, format, args);
    va_end(args);
    
    /* Add newline if not present */
    if (format[strlen(format) - 1] != '\n') {
        fprintf(out, "\n");
    }
    
    /* Flush output */
    fflush(out);
}

void pg_log_info(const char *format, ...) {
    if (PG_LOG_INFO > pg_log_config.log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    FILE *out = pg_log_config.log_file ? pg_log_config.log_file : stderr;
    
    /* Print timestamp if enabled */
    if (pg_log_config.include_timestamp) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(out, "[%s] ", timestamp);
    }
    
    /* Print log level */
    fprintf(out, "[%s] ", log_level_strings[PG_LOG_INFO]);
    
    /* Print PID if enabled */
    if (pg_log_config.include_pid) {
        fprintf(out, "[%d] ", getpid());
    }
    
    /* Print the actual message */
    vfprintf(out, format, args);
    va_end(args);
    
    /* Add newline if not present */
    if (format[strlen(format) - 1] != '\n') {
        fprintf(out, "\n");
    }
    
    /* Flush output */
    fflush(out);
}

void pg_log_debug(const char *format, ...) {
    if (PG_LOG_DEBUG > pg_log_config.log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    FILE *out = pg_log_config.log_file ? pg_log_config.log_file : stderr;
    
    /* Print timestamp if enabled */
    if (pg_log_config.include_timestamp) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(out, "[%s] ", timestamp);
    }
    
    /* Print log level */
    fprintf(out, "[%s] ", log_level_strings[PG_LOG_DEBUG]);
    
    /* Print PID if enabled */
    if (pg_log_config.include_pid) {
        fprintf(out, "[%d] ", getpid());
    }
    
    /* Print the actual message */
    vfprintf(out, format, args);
    va_end(args);
    
    /* Add newline if not present */
    if (format[strlen(format) - 1] != '\n') {
        fprintf(out, "\n");
    }
    
    /* Flush output */
    fflush(out);
}
