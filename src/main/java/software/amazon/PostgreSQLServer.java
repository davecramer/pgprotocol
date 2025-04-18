package software.amazon;

import java.io.*;
import java.net.*;
import java.nio.ByteBuffer;

public class PostgreSQLServer {
  private static final int PORT = 5432;

  public static void main(String[] args) {
    try (ServerSocket serverSocket = new ServerSocket(PORT)) {
      System.out.println("PostgreSQL Server listening on port " + PORT);

      while (true) {
        Socket clientSocket = serverSocket.accept();
        new ClientHandler(clientSocket).start();
      }
    } catch (IOException e) {
      e.printStackTrace();
    }
  }
}

class ClientHandler extends Thread {
  private Socket clientSocket;
  private DataInputStream in;
  private DataOutputStream out;

  public ClientHandler(Socket socket) {
    this.clientSocket = socket;
  }

  @Override
  public void run() {
    try {
      in = new DataInputStream(clientSocket.getInputStream());
      out = new DataOutputStream(clientSocket.getOutputStream());
      handleStartup();
    } catch (IOException e) {
      e.printStackTrace();
    } finally {
      try {
        clientSocket.close();
      } catch (IOException e) {
        e.printStackTrace();
      }
    }
  }

  private void handleStartup() throws IOException {
    // Read startup message length
    int messageLength = in.readInt();

    // Read protocol version
    int protocolVersion = in.readInt();

    // Read parameters
    byte[] params = new byte[messageLength - 8]; // subtract length and version fields
    in.readFully(params);

    // Send AuthenticationOk
    sendAuthenticationOk();

    // Send ParameterStatus messages
    sendParameterStatus("server_version", "14.0");
    sendParameterStatus("client_encoding", "UTF8");

    // Send BackendKeyData
    sendBackendKeyData(1234, 5678);

    // Send ReadyForQuery
    sendReadyForQuery();

    // Handle queries
    handleQueries();
  }

  private void sendAuthenticationOk() throws IOException {
    ByteBuffer buffer = ByteBuffer.allocate(8);
    buffer.put((byte) 'R');    // Authentication message type
    buffer.putInt(8);          // Message length
    buffer.putInt(0);          // Auth OK
    out.write(buffer.array());
  }

  private void sendParameterStatus(String name, String value) throws IOException {
    byte[] nameBytes = name.getBytes();
    byte[] valueBytes = value.getBytes();
    int msgLength = 4 + nameBytes.length + 1 + valueBytes.length + 1;

    ByteBuffer buffer = ByteBuffer.allocate(1 + msgLength);
    buffer.put((byte) 'S');    // ParameterStatus message type
    buffer.putInt(msgLength);   // Message length
    buffer.put(nameBytes);
    buffer.put((byte) 0);      // Null terminator
    buffer.put(valueBytes);
    buffer.put((byte) 0);      // Null terminator
    out.write(buffer.array());
  }

  private void sendBackendKeyData(int processId, int secretKey) throws IOException {
    ByteBuffer buffer = ByteBuffer.allocate(13);
    buffer.put((byte) 'K');    // BackendKeyData message type
    buffer.putInt(12);         // Message length
    buffer.putInt(processId);
    buffer.putInt(secretKey);
    out.write(buffer.array());
  }

  private void sendReadyForQuery() throws IOException {
    ByteBuffer buffer = ByteBuffer.allocate(6);
    buffer.put((byte) 'Z');    // ReadyForQuery message type
    buffer.putInt(5);          // Message length
    buffer.put((byte) 'I');    // Idle state
    out.write(buffer.array());
  }

  private void handleQueries() throws IOException {
    while (true) {
      byte messageType = in.readByte();
      int messageLength = in.readInt();

      switch (messageType) {
        case 'Q':  // Simple Query
          handleSimpleQuery(messageLength);
          break;
        case 'X':  // Terminate
          return;
        default:
          skipMessage(messageLength - 4);
      }
    }
  }

  private void handleSimpleQuery(int length) throws IOException {
    // Read query string
    byte[] queryBytes = new byte[length - 4];
    in.readFully(queryBytes);
    String query = new String(queryBytes).trim();

    // Send empty result set
    sendRowDescription(new String[] {"column1"});
    sendCommandComplete("SELECT 0");
    sendReadyForQuery();
  }

  private void sendRowDescription(String[] columnNames) throws IOException {
    int msgLength = 4 + 2 + (columnNames.length * 18);
    ByteBuffer buffer = ByteBuffer.allocate(1 + msgLength);
    buffer.put((byte) 'T');    // RowDescription message type
    buffer.putInt(msgLength);   // Message length
    buffer.putShort((short) columnNames.length);  // Number of fields

    for (String column : columnNames) {
      buffer.put(column.getBytes());
      buffer.put((byte) 0);   // Null terminator
      buffer.putInt(0);       // Table OID
      buffer.putShort((short) 0);  // Column number
      buffer.putInt(23);      // Data type OID (INT4)
      buffer.putShort((short) 4);  // Data type size
      buffer.putInt(-1);      // Type modifier
      buffer.putShort((short) 0);  // Format code
    }
    out.write(buffer.array());
  }

  private void sendCommandComplete(String command) throws IOException {
    byte[] commandBytes = command.getBytes();
    int msgLength = 4 + commandBytes.length + 1;

    ByteBuffer buffer = ByteBuffer.allocate(1 + msgLength);
    buffer.put((byte) 'C');    // CommandComplete message type
    buffer.putInt(msgLength);   // Message length
    buffer.put(commandBytes);
    buffer.put((byte) 0);      // Null terminator
    out.write(buffer.array());
  }

  private void skipMessage(int length) throws IOException {
    in.skipBytes(length);
  }
}
