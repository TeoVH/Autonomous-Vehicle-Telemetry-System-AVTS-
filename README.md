# Autonomous Vehicle Telemetry System (AVTS)

The **Autonomous Vehicle Telemetry System (AVTS)** is a client-server project designed to simulate real-time telemetry for an autonomous vehicle.  
It was developed as an academic project to practice **network programming, concurrency, authentication, and GUI development**.

- **Server**: Implemented in C using Berkeley sockets and pthreads.
- **Clients**: Implemented in Python (Tkinter) and Java (Swing/JavaFX).
- **Protocol**: Custom text-based protocol with authentication, telemetry broadcast, and role-based commands.

## Build & Run Instructions

### 1. Build the server
The server is written in C and requires `gcc` and `pthread`.  
From the project root:

```bash
make
```
This will produce an executable named server.

### 2. Run the server

```bash
./server <port> <log_file>
```

**Example:**
```bash
./server 5050 log.txt
```

- The server will listen on the specified port.
- Logs will be written both to the console and the log file.

### Connect a client
During development, you can use **netcat (nc)** to test the protocol:

```bash
nc 127.0.0.1 5050
```

Example login as admin:
```bash
LOGIN ADMIN root password
```

LOGIN ADMIN root password
```bash
LOGIN OBSERVER juan 12345
```
