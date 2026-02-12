# Description
This project is a Community-led World Cup subscription system, where C++ Clients can subscribe to game channels, report, and receive updates from one another about the games.
A Java Server acts as a message broker, routing reports between clients using the STOMP protocol, while maintaining a database of users' activity, which is implemented in SQLite.


# Architecture
C++ Client communicates STOMP frames over TCP to a Java server that handles all clients, and records user activity to a SQLite database through a Python SQL server.

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
- This class holds the core protocol logic for handling each client's communication with the server. Each client gets its own instance.
- Receives a raw STOMP frame, parses it into its command, headers, and body.
- Determines the appropriate action (e.g., connecting, subscribing, sending messages, disconnecting).
- Builds and sends the corresponding response frame back to the client through the ConnectionsImpl.


## Client Side
**Framework (provided)**
- The client is built on top of a course-provided networking and parsing framework.
  - `ConnectionHandler`: handles low-level TCP socket communication - sending and receiving bytes/frames to/from the server.
  - `Event`: defines the Event class and parses game event JSON files into Event objects using the nlohmann/json library.

**StompClient.cpp**
- This class is the main entry point for the client application.
- Runs two threads: one reading keyboard input from the user, and one reading frames from the server.
- Both threads share the same StompProtocol instance, which handles all logic.
- Supports multiple login sessions - after logout, the client returns to the login prompt without restarting the application.

**StompProtocol.cpp**
- This class holds the client-side protocol's logic - mirrors the server's protocol, but from the client's perspective.
- Translates keyboard commands (login, join, exit, report, summary, logout) into STOMP frames and sends them to the server.
- Parses incoming server frames (CONNECTED, MESSAGE, RECEIPT, ERROR) and updates local game data or prints output to the user.
- Maintains local storage of game event reports per user, enabling the summary command to write game summaries to a file.

## Database
**Framework (provided)**
- A Python SQL server listens on a socket and executes SQL commands against a SQLite database.
- The Java `Database` class sends SQL strings over a TCP socket to this Python server.
- Tracks user registrations, login/logout timestamps, and file uploads reported by clients.


# How to Run
- Requires: Java + Maven, C++ with Boost library, Python 3.
- Start the components in the following order:\
**SQL Server:**
```bash
python3 data/sql_server.py
```

**Java Server (TPC):**
```bash
cd server
mvn compile
mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer" -Dexec.args="7777 tpc"
```
**Java Server (Reactor):**
```bash
mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer" -Dexec.args="7777 reactor"
```

**C++ Client:**
```bash
cd client
make
./bin/StompWCIClient
```
