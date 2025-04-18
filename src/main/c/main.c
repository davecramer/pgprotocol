/**
 * main.c
 * PostgreSQL Protocol Server Emulator - Main Entry Point
 * 
 * This file contains the main entry point for the PostgreSQL protocol server emulator.
 */

#include "pg_server.h"
#include "pg_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

/* Global variables */
static PGServer *g_server = NULL;

/* Forward declarations */
static void signal_handler(int sig);
static void print_usage(const char *program_name);
static int parse_arguments(int argc, char **argv, PGServerConfig *config);

/**
 * Signal handler
 * 
 * @param sig Signal number
 */
static void signal_handler(int sig) {
    printf("Received signal %d\n", sig);
    
    if (g_server) {
        pg_server_stop(g_server);
    }
}

/**
 * Print usage information
 * 
 * @param program_name Program name
 */
static void print_usage(const char *program_name) {
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

/**
 * Parse command line arguments
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @param config Server configuration
 * @return 0 on success, -1 on error
 */
static int parse_arguments(int argc, char **argv, PGServerConfig *config) {
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
    
    // Set default values
    config->host = "127.0.0.1";
    config->port = 5432;
    config->data_dir = ".";
    config->log_file = NULL;
    config->max_connections = 100;
    config->ssl_enabled = false;
    config->ssl_cert = NULL;
    config->ssl_key = NULL;
    
    while ((c = getopt_long(argc, argv, "h:p:d:l:m:sc:k:v?", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                config->host = optarg;
                break;
            
            case 'p':
                config->port = atoi(optarg);
                break;
            
            case 'd':
                config->data_dir = optarg;
                break;
            
            case 'l':
                config->log_file = optarg;
                break;
            
            case 'm':
                config->max_connections = atoi(optarg);
                break;
            
            case 's':
                config->ssl_enabled = true;
                break;
            
            case 'c':
                config->ssl_cert = optarg;
                break;
            
            case 'k':
                config->ssl_key = optarg;
                break;
            
            case 'v':
                // Enable verbose logging
                break;
            
            case '?':
                print_usage(argv[0]);
                return -1;
            
            default:
                fprintf(stderr, "Unknown option: %c\n", c);
                return -1;
        }
    }
    
    return 0;
}

/**
 * Main entry point
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char **argv) {
    PGServerConfig config;
    
    // Parse command line arguments
    if (parse_arguments(argc, argv, &config) != 0) {
        return 1;
    }
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create server
    g_server = pg_server_create(&config);
    if (!g_server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }
    
    // Start server
    if (pg_server_start(g_server) != 0) {
        fprintf(stderr, "Failed to start server\n");
        pg_server_destroy(g_server);
        return 1;
    }
    
    printf("PostgreSQL protocol server emulator started on %s:%d\n", config.host, config.port);
    printf("Press Ctrl+C to stop\n");
    
    // Run server
    int result = pg_server_run(g_server);
    
    // Clean up
    pg_server_destroy(g_server);
    g_server = NULL;
    
    return result == 0 ? 0 : 1;
}
