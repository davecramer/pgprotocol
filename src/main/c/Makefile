CC = gcc
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = -L /usr/local/opt/openssl@3/lib/ -lssl -lcrypto

SRCS = main.c pg_server.c pg_log.c
OBJS = $(SRCS:.c=.o)
TARGET = pg_server

# Protocol logging version
LOGGING_SRCS = pg_server.c pg_log.c pg_protocol_logging.c pg_server_main.c
LOGGING_OBJS = $(LOGGING_SRCS:.c=.o)
LOGGING_TARGET = pg_server_with_logging

.PHONY: all clean logging

all: $(TARGET) $(LOGGING_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(LOGGING_TARGET): $(LOGGING_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LOGGING_OBJS) $(TARGET) $(LOGGING_TARGET)

logging: $(LOGGING_TARGET)
