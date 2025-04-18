# PostgreSQL Protocol Server Emulator

This project implements a server that emulates the PostgreSQL wire protocol. It can be used for testing PostgreSQL clients, developing protocol extensions, or learning about the PostgreSQL protocol.

## Features

- Full implementation of the PostgreSQL wire protocol (version 3.0)
- Support for basic authentication methods (MD5, cleartext)
- Handling of simple queries (SELECT, INSERT, UPDATE, DELETE)
- Transaction management (BEGIN, COMMIT, ROLLBACK)
- Extensible through callback functions for authentication and query handling
- Optional SSL support

## Building

To build the server, you need:

- GCC or compatible C compiler
- OpenSSL development libraries
- Make

Run the following command to build the server:

```bash
make
```

## Usage

```bash
./pg_server [options]
```

### Options

- `-h, --host HOST`: Host to bind to (default: 127.0.0.1)
- `-p, --port PORT`: Port to listen on (default: 5432)
- `-d, --data-dir DIR`: Data directory (default: .)
- `-l, --log-file FILE`: Log file (default: stderr)
- `-m, --max-conn NUM`: Maximum number of connections (default: 100)
- `-s, --ssl`: Enable SSL
- `-c, --ssl-cert FILE`: SSL certificate file
- `-k, --ssl-key FILE`: SSL key file
- `-v, --verbose`: Enable verbose logging
- `-?, --help`: Show help message

## Protocol Implementation

The server implements the PostgreSQL protocol version 3.0, which is used by PostgreSQL 7.4 and later. The protocol is message-based, with each message consisting of a type byte followed by a length and payload.

### Supported Frontend Messages

- Startup message (protocol negotiation)
- Query message (simple query protocol)
- Terminate message (client disconnect)
- Password message (authentication response)

### Supported Backend Messages

- Authentication messages (OK, MD5, cleartext)
- Error response
- Notice response
- Ready for query
- Row description
- Data row
- Command complete
- Parameter status
- Backend key data
- Empty query response

## Extending the Server

The server can be extended through callback functions for authentication and query handling. See the `pg_server.h` header file for details.

### Authentication Callback

```c
typedef int (*PGAuthCallback)(PGClientConn *client, const char *user, const char *password);
```

### Query Callback

```c
typedef int (*PGQueryCallback)(PGClientConn *client, const char *query);
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## References

- [PostgreSQL Documentation: Frontend/Backend Protocol](https://www.postgresql.org/docs/current/protocol.html)
- [PostgreSQL Documentation: Message Flow](https://www.postgresql.org/docs/current/protocol-flow.html)
- [PostgreSQL Documentation: Message Formats](https://www.postgresql.org/docs/current/protocol-message-formats.html)
