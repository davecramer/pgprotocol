/**
 * pg_log.h
 * Logging functionality for PostgreSQL Protocol Server Emulator
 */

#ifndef PG_LOG_H
#define PG_LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/* Log levels */
typedef enum {
    PG_LOG_ERROR = 0,
    PG_LOG_WARNING = 1,
    PG_LOG_INFO = 2,
    PG_LOG_DEBUG = 3
} PGLogLevel;

/* Log configuration */
typedef struct {
    FILE *log_file;          /* File to write logs to (NULL for stderr) */
    PGLogLevel log_level;    /* Current log level */
    int include_timestamp;   /* Whether to include timestamp in log messages */
    int include_pid;         /* Whether to include process ID in log messages */
} PGLogConfig;

/* Global log configuration */
extern PGLogConfig pg_log_config;

/* Initialize logging */
void pg_log_init(FILE *log_file, PGLogLevel level);

/* Set log level */
void pg_log_set_level(PGLogLevel level);

/* Set log file */
void pg_log_set_file(FILE *log_file);

/* Close logging */
void pg_log_close(void);

/* Log functions */
void pg_log_error(const char *format, ...);
void pg_log_warning(const char *format, ...);
void pg_log_info(const char *format, ...);
void pg_log_debug(const char *format, ...);

/* Generic log function */
void pg_log(PGLogLevel level, const char *format, ...);

#endif /* PG_LOG_H */
