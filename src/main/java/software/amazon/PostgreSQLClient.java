package software.amazon;

import java.io.*;
import java.net.*;
import java.nio.ByteBuffer;

import java.util.HashMap;
import java.util.Map;

/**
 * PostgreSQL protocol message types
 */
class PgMessageType {
  // Frontend message types
  public static final byte QUERY = 'Q';
  public static final byte PARSE = 'P';
  public static final byte BIND = 'B';
  public static final byte DESCRIBE = 'D';
  public static final byte EXECUTE = 'E';
  public static final byte SYNC = 'S';
  public static final byte TERMINATE = 'X';
  public static final byte COPY_DATA = 'd';
  public static final byte COPY_DONE = 'c';
  public static final byte FUNCTION_CALL = 'F';

  // Backend message types
  public static final byte AUTHENTICATION = 'R';
  public static final byte PARAMETER_STATUS = 'S';
  public static final byte BACKEND_KEY_DATA = 'K';
  public static final byte READY_FOR_QUERY = 'Z';
  public static final byte ROW_DESCRIPTION = 'T';
  public static final byte DATA_ROW = 'D';
  public static final byte COMMAND_COMPLETE = 'C';
  public static final byte ERROR_RESPONSE = 'E';
  public static final byte EMPTY_QUERY_RESPONSE = 'I';
  public static final byte PARSE_COMPLETE = '1';
  public static final byte BIND_COMPLETE = '2';
  public static final byte CLOSE_COMPLETE = '3';
  public static final byte NO_DATA = 'n';
  public static final byte PORTAL_SUSPENDED = 's';
  public static final byte PARAMETER_DESCRIPTION = 't';
  public static final byte FUNCTION_CALL_RESPONSE = 'V';
}

public class PostgreSQLClient {
  private Socket socket;
  private DataInputStream in;
  private DataOutputStream out;

  public static void main(String[] args) {
    PostgreSQLClient client = new PostgreSQLClient();
    try {
      // Basic connection test
      client.connect("localhost", 5433, "testdb", "user", "password");

      // Test simple query
      System.out.println("\n=== Testing Simple Query ===");
      client.executeQuery("SELECT * FROM test");

      // Test extended query protocol
      System.out.println("\n=== Testing Extended Query Protocol ===");
      client.testExtendedQuery();

      // Test prepared statements
      System.out.println("\n=== Testing Prepared Statements ===");
      client.testPreparedStatements();

      // Test COPY protocol
      System.out.println("\n=== Testing COPY Protocol ===");
      client.testCopyProtocol();

      // Test function calls
      System.out.println("\n=== Testing Function Calls ===");
      client.testFunctionCall();

      // Test cancellation
      System.out.println("\n=== Testing Query Cancellation ===");
      client.testCancellation();

      client.disconnect();
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  public void connect(String host, int port, String database, String user, String password)
      throws IOException {
    socket = new Socket(host, port);
    in = new DataInputStream(socket.getInputStream());
    out = new DataOutputStream(socket.getOutputStream());

    // Send startup message
    sendStartupMessage(database, user);

    // Handle server response
    handleStartupResponse();
  }

  private void sendStartupMessage(String database, String user) throws IOException {
    // Protocol version 3.0
    Map<String, String> params = new HashMap<>();
    params.put("user", user);
    params.put("database", database);

    // Calculate message size
    int messageSize = 4 + 4; // Length + protocol version
    for (Map.Entry<String, String> param : params.entrySet()) {
      messageSize += param.getKey().length() + 1;
      messageSize += param.getValue().length() + 1;
    }
    messageSize += 1; // Final null byte

    ByteBuffer buffer = ByteBuffer.allocate(messageSize);
    buffer.putInt(messageSize);
    buffer.putInt(196608); // Protocol version 3.0 (3 << 16)

    for (Map.Entry<String, String> param : params.entrySet()) {
      buffer.put(param.getKey().getBytes());
      buffer.put((byte) 0);
      buffer.put(param.getValue().getBytes());
      buffer.put((byte) 0);
    }
    buffer.put((byte) 0);

    out.write(buffer.array());
  }

  private void handleStartupResponse() throws IOException {
    while (true) {
      byte messageType = in.readByte();
      int messageLength = in.readInt() - 4;
      byte[] message = new byte[messageLength];
      in.readFully(message);

      switch (messageType) {
        case PgMessageType.AUTHENTICATION:
          handleAuthentication(message);
          break;
        case PgMessageType.PARAMETER_STATUS:
          handleParameterStatus(message);
          break;
        case PgMessageType.BACKEND_KEY_DATA:
          handleBackendKeyData(message);
          break;
        case PgMessageType.READY_FOR_QUERY:
          return;
        default:
          System.out.println("Unknown message type: " + (char) messageType);
      }
    }
  }

  public void executeQuery(String query) throws IOException {
    // Send Query message
    byte[] queryBytes = query.getBytes();
    int messageSize = 4 + queryBytes.length + 1;

    ByteBuffer buffer = ByteBuffer.allocate(1 + messageSize);
    buffer.put(PgMessageType.QUERY);
    buffer.putInt(messageSize);
    buffer.put(queryBytes);
    buffer.put((byte) 0);

    out.write(buffer.array());

    // Handle response
    handleQueryResponse();
  }

  private void handleQueryResponse() throws IOException {
    while (true) {
      byte messageType = in.readByte();
      int messageLength = in.readInt() - 4;
      byte[] message = new byte[messageLength];
      in.readFully(message);

      switch (messageType) {
        case PgMessageType.ROW_DESCRIPTION:
          handleRowDescription(message);
          break;
        case PgMessageType.DATA_ROW:
          handleDataRow(message);
          break;
        case PgMessageType.COMMAND_COMPLETE:
          handleCommandComplete(message);
          break;
        case PgMessageType.EMPTY_QUERY_RESPONSE:
          System.out.println("Empty query response");
          break;
        case PgMessageType.READY_FOR_QUERY:
          return;
        default:
          System.out.println("Unknown message type: " + (char) messageType);
      }
    }
  }

  private void handleAuthentication(byte[] message) {
    int authType = ByteBuffer.wrap(message).getInt();
    System.out.println("Authentication type: " + authType);
  }

  private void handleParameterStatus(byte[] message) {
    String[] params = new String(message).split("\0");
    System.out.println("Parameter: " + params[0] + " = " + params[1]);
  }

  private void handleBackendKeyData(byte[] message) {
    ByteBuffer buffer = ByteBuffer.wrap(message);
    int processId = buffer.getInt();
    int secretKey = buffer.getInt();
    System.out.println("Backend Key Data - PID: " + processId + ", Secret Key: " + secretKey);
  }

  private void handleRowDescription(byte[] message) {
    ByteBuffer buffer = ByteBuffer.wrap(message);
    short fieldCount = buffer.getShort();
    System.out.println("Number of fields: " + fieldCount);
    // Parse field descriptions here
  }

  private void handleDataRow(byte[] message) {
    System.out.println("Data row received");
    // Parse row data here
  }

  private void handleCommandComplete(byte[] message) {
    String command = new String(message).trim();
    System.out.println("Command complete: " + command);
  }

  public void disconnect() throws IOException {
    // Send Terminate message
    ByteBuffer buffer = ByteBuffer.allocate(5);
    buffer.put(PgMessageType.TERMINATE);
    buffer.putInt(4);
    out.write(buffer.array());

    socket.close();
  }

  public void testExtendedQuery() throws IOException {
    // Parse phase
    sendParse("myStatement", "SELECT * FROM test WHERE id = $1", new int[]{23});

    // Bind phase
    int[] paramValues = {1};
    sendBind("myPortal", "myStatement", paramValues);

    // Describe phase
    sendDescribe('P', "myPortal");

    // Execute phase
    sendExecute("myPortal", 0);

    // Sync
    sendSync();

    // Handle responses
    handleExtendedQueryResponse();
  }

  public void testPreparedStatements() throws IOException {
    // Prepare statement
    sendParse("stmt1", "SELECT * FROM test WHERE id = $1", new int[]{23});

    // Execute multiple times with different parameters
    for (int i = 1; i <= 3; i++) {
      sendBind("", "stmt1", new int[]{i});
      sendExecute("", 0);
    }

    sendSync();
    handleExtendedQueryResponse();
  }

  public void testCopyProtocol() throws IOException {
    // Test COPY FROM
    executeQuery("COPY test FROM STDIN");
    sendCopyData("1\ttest1\n");
    sendCopyData("2\ttest2\n");
    sendCopyDone();

    // Test COPY TO
    executeQuery("COPY test TO STDOUT");
    handleCopyOutResponse();
  }

  public void testFunctionCall() throws IOException {
    // Function call message
    int functionOid = 1234;
    byte[][] arguments = new byte[][]{"arg1".getBytes()};
    sendFunctionCall(functionOid, arguments);
    handleFunctionCallResponse();
  }

  public void testCancellation() throws IOException {
    // Send cancel request using backend key data
    sendCancelRequest();
  }

  // New protocol-specific send methods

  private void sendParse(String statementName, String query, int[] paramTypes) throws IOException {
    ByteBuffer buffer = ByteBuffer.allocate(1024);
    buffer.put(PgMessageType.PARSE);

    int startPos = buffer.position();
    buffer.putInt(0); // Length placeholder

    buffer.put(statementName.getBytes());
    buffer.put((byte) 0);
    buffer.put(query.getBytes());
    buffer.put((byte) 0);

    buffer.putShort((short) paramTypes.length);
    for (int paramType : paramTypes) {
      buffer.putInt(paramType);
    }

    // Update message length
    int length = buffer.position() - startPos;
    buffer.putInt(startPos, length);

    out.write(buffer.array(), 0, buffer.position());
  }

  private void sendBind(String portalName, String statementName, int[] paramValues)
      throws IOException {
    ByteBuffer buffer = ByteBuffer.allocate(1024);
    buffer.put(PgMessageType.BIND);

    int startPos = buffer.position();
    buffer.putInt(0); // Length placeholder

    buffer.put(portalName.getBytes());
    buffer.put((byte) 0);
    buffer.put(statementName.getBytes());
    buffer.put((byte) 0);

    // Parameter format codes
    buffer.putShort((short) 1);
    buffer.putShort((short) 1); // Use binary format

    // Parameter values
    buffer.putShort((short) paramValues.length);
    for (int value : paramValues) {
      buffer.putInt(4); // Value length
      buffer.putInt(value);
    }

    // Result format codes
    buffer.putShort((short) 1);
    buffer.putShort((short) 1); // Use binary format

    // Update message length
    int length = buffer.position() - startPos;
    buffer.putInt(startPos, length);

    out.write(buffer.array(), 0, buffer.position());
  }

  private void sendDescribe(char type, String name) throws IOException {
    ByteBuffer buffer = ByteBuffer.allocate(1024);
    buffer.put(PgMessageType.DESCRIBE);
    buffer.putInt(6 + name.length());
    buffer.put((byte) type);
    buffer.put(name.getBytes());
    buffer.put((byte) 0);
    out.write(buffer.array(), 0, buffer.position());
  }

  private void sendExecute(String portalName, int maxRows) throws IOException {
    ByteBuffer buffer = ByteBuffer.allocate(1024);
    buffer.put(PgMessageType.EXECUTE);
    buffer.putInt(4 + portalName.length() + 1 + 4);
    buffer.put(portalName.getBytes());
    buffer.put((byte) 0);
    buffer.putInt(maxRows);
    out.write(buffer.array(), 0, buffer.position());
  }

  private void sendSync() throws IOException {
    ByteBuffer buffer = ByteBuffer.allocate(5);
    buffer.put(PgMessageType.SYNC);
    buffer.putInt(4);
    out.write(buffer.array());
  }

  private void sendCopyData(String data) throws IOException {
    byte[] bytes = data.getBytes();
    ByteBuffer buffer = ByteBuffer.allocate(5 + bytes.length);
    buffer.put(PgMessageType.COPY_DATA);
    buffer.putInt(4 + bytes.length);
    buffer.put(bytes);
    out.write(buffer.array());
  }

  private void sendCopyDone() throws IOException {
    ByteBuffer buffer = ByteBuffer.allocate(5);
    buffer.put(PgMessageType.COPY_DONE);
    buffer.putInt(4);
    out.write(buffer.array());
  }

  private void sendFunctionCall(int functionOid, byte[][] arguments) throws IOException {
    // Calculate total size
    int totalSize = 4 + 4 + 2 + 2;
    for (byte[] arg : arguments) {
      totalSize += 4 + (arg != null ? arg.length : 0);
    }

    ByteBuffer buffer = ByteBuffer.allocate(1 + totalSize);
    buffer.put(PgMessageType.FUNCTION_CALL);
    buffer.putInt(totalSize);
    buffer.putInt(functionOid);

    buffer.putShort((short) 1); // Number of argument format codes
    buffer.putShort((short) 1); // Binary format

    // Arguments
    buffer.putShort((short) arguments.length);
    for (byte[] arg : arguments) {
      if (arg == null) {
        buffer.putInt(-1); // Null value
      } else {
        buffer.putInt(arg.length);
        buffer.put(arg);
      }
    }

    out.write(buffer.array(), 0, buffer.position());
  }

  private void sendCancelRequest() throws IOException {
    // This would typically be sent on a new connection
    System.out.println("Sending cancel request (simulated)");
  }

  private void handleCopyOutResponse() throws IOException {
    System.out.println("Handling COPY OUT response");
    // Process COPY data messages until completion
    while (true) {
      byte messageType = in.readByte();
      int messageLength = in.readInt() - 4;
      byte[] message = new byte[messageLength];
      in.readFully(message);

      if (messageType == PgMessageType.COPY_DATA) { // Copy Data
        System.out.println("COPY data: " + new String(message));
      } else if (messageType == PgMessageType.COPY_DONE) { // Copy Done
        System.out.println("COPY completed");
        break;
      }
    }
  }

  private void handleFunctionCallResponse() throws IOException {
    byte messageType = in.readByte();
    int messageLength = in.readInt() - 4;
    byte[] message = new byte[messageLength];
    in.readFully(message);

    if (messageType == PgMessageType.FUNCTION_CALL_RESPONSE) {
      System.out.println("Function call response received");
      // Process function result
    }
  }

  private void handleExtendedQueryResponse() throws IOException {
    boolean done = false;
    while (!done) {
      byte messageType = in.readByte();
      int messageLength = in.readInt() - 4;
      byte[] message = new byte[messageLength];
      in.readFully(message);

      switch (messageType) {
        case PgMessageType.PARSE_COMPLETE:
          System.out.println("Parse complete");
          break;
        case PgMessageType.BIND_COMPLETE:
          System.out.println("Bind complete");
          break;
        case PgMessageType.CLOSE_COMPLETE:
          System.out.println("Close complete");
          break;
        case PgMessageType.ROW_DESCRIPTION:
          handleRowDescription(message);
          break;
        case PgMessageType.DATA_ROW:
          handleDataRow(message);
          break;
        case PgMessageType.COMMAND_COMPLETE:
          handleCommandComplete(message);
          break;
        case PgMessageType.ERROR_RESPONSE:
          handleErrorResponse(message);
          break;
        case PgMessageType.EMPTY_QUERY_RESPONSE:
          System.out.println("Empty query response");
          break;
        case PgMessageType.NO_DATA:
          System.out.println("No data");
          break;
        case PgMessageType.PORTAL_SUSPENDED:
          System.out.println("Portal suspended");
          break;
        case PgMessageType.PARAMETER_DESCRIPTION:
          handleParameterDescription(message);
          break;
        case PgMessageType.READY_FOR_QUERY:
          System.out.println("Ready for query: " + (char) message[0]);
          done = true;
          break;
        default:
          System.out.println(
              "Unknown message type in extended query response: " + (char) messageType);
      }
    }
  }

  private void handleParameterDescription(byte[] message) {
    ByteBuffer buffer = ByteBuffer.wrap(message);
    short paramCount = buffer.getShort();
    System.out.println("Parameter description - count: " + paramCount);

    for (int i = 0; i < paramCount; i++) {
      int paramType = buffer.getInt();
      System.out.println("  Parameter " + i + " type: " + paramType);
    }
  }

  private void handleErrorResponse(byte[] message) {
    ByteBuffer buffer = ByteBuffer.wrap(message);
    StringBuilder errorBuilder = new StringBuilder("ERROR: ");

    byte fieldType;
    while ((fieldType = buffer.get()) != 0) {
      StringBuilder valueBuilder = new StringBuilder();
      byte b;
      while ((b = buffer.get()) != 0) {
        valueBuilder.append((char) b);
      }

      String value = valueBuilder.toString();
      switch (fieldType) {
        case 'S': // Severity
          errorBuilder.append("Severity: ").append(value).append(", ");
          break;
        case 'C': // Code
          errorBuilder.append("Code: ").append(value).append(", ");
          break;
        case 'M': // Message
          errorBuilder.append("Message: ").append(value);
          break;
      }
    }

    System.out.println(errorBuilder.toString());
  }
}


