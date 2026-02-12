# Description
This project is a Community-led World Cup subscription system, where C++ Clients can subscribe to game channels, report, and receive updates from one another about the games.
A Java Server acts as a message broker, routing reports between clients using the STOMP protocol, while maintaining a database of users' activity, which is implemented in SQLite.

# Architecture
C++ Client communicates STOMP frames over TCP to a Java server that handles all clients, and sends SQL strings of user activity documentation to a Python SQL server that operates a SQLite database.

## Server Side
**Framework (provided)**
- The server is built on top of a course-provided networking framework that supports two server patterns:
  - Thread-Per-Client (TPC): spawns a dedicated thread for each connected client.
  - Reactor: uses a single selector thread with a thread pool for non-blocking I/O.
- Both patterns share the same protocol and connections logic, so switching between them requires no code changes - just a startup argument.
- Key framework classes: `BaseServer`, `Reactor`, `BlockingConnectionHandler`, `NonBlockingConnectionHandler`, `ActorThreadPool`, `Server`, `ConnectionHandler`.

**ConnectionsImpl.java**
- This class manages the connection with all clients, pairing each client with a connection handler responsible for sending/receiving messages to/from other clients via the server.
- I used three ConcurrentHashMaps to manage the clients' connection handlers and to capture the relationship between clients and game channels.
- These maps are shared across client threads, so I used ConcurrentHashMaps to ensure thread safety.
  - `Connection ID → ConnectionHandler`: allows lookup of a client's handler by their ID.
  - `Channel → (Connection ID, Subscription ID)`: getting a channel's subscribers' Connection IDs and their Subscription ID.
  - `Connection ID → (Subscription ID, Channel)`: getting a client's subscriptions.

**StompMessagingProtocolImpl.java**
- The core protocol logic for handling each client's communication with the server. Each client gets its own instance.
- Receives a raw STOMP frame, parses it into its command, headers, and body.
- Determines the appropriate action (e.g., connecting, subscribing, sending messages, disconnecting).
- Builds and sends the corresponding response frame back to the client through the ConnectionsImpl.

