/**
 * pg_server_main.c
 * Main entry point for PostgreSQL Protocol Server Emulator
 */

#include "pg_server.h"
#include "pg_log.h"
#include "pg_protocol_logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

// Global server instance for signal handling
PGServer *g_server = NULL;

// Signal handler
void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        pg_log_info("Received signal %d, shutting down", sig);
        if (g_server) {
            pg_server_stop(g_server);
        }
    }
}

// Print usage information
void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -h, --host HOST       Host to bind to (default: 127.0.0.1)\n");
    printf("  -p, --port PORT       Port to listen on (default: 5432)\n");
    printf("  -d, --data-dir DIR    Data directory (default: .)\n");
    printf("  -l, --log-file FILE   Log file (default: stderr)\n");
    printf("  -m, --max-conn NUM    Maximum number of connections (default: 100)\n");
    printf("  -s, --ssl             Enable SSL\n");
    printf("  -c, --ssl-cert FILE   SSL certificate file\n");
    printf("  -k, --ssl-key FILE    SSL key file\n");
    printf("  -v, --verbose         Enable verbose logging\n");
    printf("  -?, --help            Show this help message\n");
}

int main(int argc, char *argv[]) {
    // Default configuration
    PGServerConfig config = {
        .host = "127.0.0.1",
        .port = 5432,
        .data_dir = ".",
        .log_file = NULL,
        .max_connections = 100,
        .ssl_enabled = false,
        .ssl_cert = NULL,
        .ssl_key = NULL,
        .verbose = false
    };

    // Parse command line options
    static struct option long_options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"data-dir", required_argument, 0, 'd'},
        {"log-file", required_argument, 0, 'l'},
        {"max-conn", required_argument, 0, 'm'},
        {"ssl", no_argument, 0, 's'},
        {"ssl-cert", required_argument, 0, 'c'},
        {"ssl-key", required_argument, 0, 'k'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, '?'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "h:p:d:l:m:sc:k:v?", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                config.host = optarg;
                break;
            case 'p':
                config.port = atoi(optarg);
                break;
            case 'd':
                config.data_dir = optarg;
                break;
            case 'l':
                config.log_file = optarg;
                break;
            case 'm':
                config.max_connections = atoi(optarg);
                break;
            case 's':
                config.ssl_enabled = true;
                break;
            case 'c':
                config.ssl_cert = optarg;
                break;
            case 'k':
                config.ssl_key = optarg;
                break;
            case 'v':
                config.verbose = true;
                break;
            case '?':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // Initialize logging
    FILE *log_file = NULL;
    if (config.log_file) {
        log_file = fopen(config.log_file, "a");
        if (!log_file) {
            fprintf(stderr, "Error: Could not open log file '%s'\n", config.log_file);
            return 1;
        }
    }
    
    pg_log_init(log_file ? log_file : stderr, config.verbose ? PG_LOG_DEBUG : PG_LOG_INFO);
    pg_log_info("PostgreSQL Protocol Server Emulator starting up");
    pg_log_info("Configuration:");
    pg_log_info("  Host: %s", config.host);
    pg_log_info("  Port: %d", config.port);
    pg_log_info("  Data directory: %s", config.data_dir);
    pg_log_info("  Max connections: %d", config.max_connections);
    pg_log_info("  SSL enabled: %s", config.ssl_enabled ? "yes" : "no");
    if (config.ssl_enabled) {
        pg_log_info("  SSL certificate: %s", config.ssl_cert ? config.ssl_cert : "(none)");
        pg_log_info("  SSL key: %s", config.ssl_key ? config.ssl_key : "(none)");
    }
    pg_log_info("  Verbose logging: %s", config.verbose ? "yes" : "no");

    // Set up signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Create and start server
    g_server = pg_server_create(&config);
    if (!g_server) {
        pg_log_error("Failed to create server");
        return 1;
    }

    // Set up protocol logging callbacks
    pg_server_set_logging_callbacks(g_server);

    if (pg_server_start(g_server) < 0) {
        pg_log_error("Failed to start server");
        pg_server_destroy(g_server);
        return 1;
    }

    // Run server
    pg_server_run(g_server);

    // Clean up
    pg_server_destroy(g_server);
    pg_log_info("Server shutdown complete");
    
    return 0;
}
