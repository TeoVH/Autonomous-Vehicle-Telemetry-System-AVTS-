# Autonomous Vehicle Telemetry System (AVTS)

The **Autonomous Vehicle Telemetry System (AVTS)** is a client-server project designed to simulate real-time telemetry for an autonomous vehicle.  
It was developed as an academic project to practice **network programming, concurrency, authentication, and GUI development**.

- **Server**: Implemented in C using Berkeley sockets and pthreads.
- **Clients**: Implemented in Python (Tkinter) and Java (Swing/JavaFX).
- **Protocol**: Custom text-based protocol with authentication, telemetry broadcast, and role-based commands.

The system simulates an autonomous vehicle that:
- **Transmits telemetry data** every 10 seconds to all connected users
- **Receives control commands** from authenticated administrators
- **Manages user authentication** with role-based access control
- **Logs all activities** for audit and debugging purposes

##  Architecture

- **Server**: Implemented in C using Berkeley sockets and pthreads
- **Protocol**: Custom text-based protocol with authentication and telemetry broadcast
- **Concurrency**: Multi-threaded server supporting up to 100 simultaneous clients
- **Security**: Role-based authentication with login attempt limiting
- **Logging**: Comprehensive audit trail with user action tracking

##  User Roles

### **Administrator (ADMIN)**
- Can send control commands to the vehicle
- Can request vehicle status
- Full access to all system functions
- Commands: `SPEED UP`, `TURN LEFT`, `TURN RIGHT`, `BATTERY RESET`, `STATUS`

### **Observer (OBSERVER)**
- Receives telemetry data broadcasts
- Can request vehicle status
- Read-only access to vehicle information
- Commands: `STATUS` only

##  Build & Run Instructions

### 1. Build the server
The server is written in C and requires `gcc`, `pthread`, and `libssl-dev`.  
From the project root:

```bash
make
```
This will produce an executable named `server`.

### 2. Run the server

```bash
./server <port> <log_file>
```

**Example:**
```bash
./server 5050 log.txt
```

- The server will listen on the specified port
- Logs will be written both to the console and the log file
- Telemetry data is broadcast every 10 seconds

### 3. Connect a client
During development, you can use **netcat (nc)** to test the protocol:

```bash
nc 127.0.0.1 5050
```

##  Authentication & Security

### ** Secure Password Hashing**
The system uses **SHA-256 + salt** for password security:
- Passwords are never stored in plain text
- Each user has a unique salt (64-character hex)
- Passwords are hashed using `SHA-256(salt + password)`
- Configuration stored in `config.json`

### ** User Management**

#### **Default Users (for testing):**
- **ADMIN**: `root` / `password`
- **OBSERVER**: `juan` / `12345`

#### **Creating New Users:**
```bash
# Generate hash for new user
python3 tools/hash_password.py --user newuser --role ADMIN --password 'newpassword'

# Add to config.json
{
  "users": [
    {
      "role": "ADMIN",
      "username": "newuser", 
      "salt": "generated_salt_here",
      "hash": "generated_hash_here"
    }
  ]
}
```

#### **Generate Example Configuration:**
```bash
# Create config.json.example with default users
python3 tools/hash_password.py --example > config.json.example

# Copy to working config
cp config.json.example config.json
```

### ** Login Process**
Clients still send plain text passwords (protocol unchanged):
```bash
LOGIN ADMIN root password
LOGIN OBSERVER juan 12345
```

The server compares against stored hashes internally.

## 📡 Protocol Commands

### **Administrator Commands:**
```bash
SPEED UP          # Increase vehicle speed
TURN LEFT         # Turn vehicle left
TURN RIGHT        # Turn vehicle right
BATTERY RESET     # Reset battery to 100%
STATUS            # Get current vehicle status
```

### **Observer Commands:**
```bash
STATUS            # Get current vehicle status
```

### **Server Responses:**
```bash
OK\n              # Command executed successfully
ERR locked\n      # Account locked after 3 failed attempts
ERR invalid (1/3)\n # Invalid credentials (shows attempt count)
ERR observer_only\n # Observer tried to use admin command
ERR unknown_cmd\n   # Unknown command
```

##  Telemetry Data Format

The server broadcasts telemetry data every 10 seconds in the format:
```
DATA <speed> <battery> <direction> <temperature>
```

**Example:**
```
DATA 75.5 85 FORWARD 28.3
```

- **speed**: Vehicle speed (0-100 + offset)
- **battery**: Battery level (0-100, decreases 5% every 10s)
- **direction**: FORWARD, LEFT, RIGHT
- **temperature**: Temperature in Celsius (20-40°C simulated)

##  Logging System

The system provides comprehensive logging with three types of entries:

### **1. Connection Events:**
```
[2025-09-28T12:30:45] CONNECTION [CONNECT] [unknown:NONE] from 127.0.0.1:39204
[2025-09-28T12:30:50] CONNECTION [LOGIN_SUCCESS] [root:ADMIN] from 127.0.0.1:39204
[2025-09-28T12:31:10] CONNECTION [DISCONNECT] [root:ADMIN] from 127.0.0.1:39204
```

### **2. User Actions:**
```
[2025-09-28T12:30:55] USER_ACTION [root:ADMIN] SPEED UP: speed_offset increased
[2025-09-28T12:31:00] USER_ACTION [root:ADMIN] TURN LEFT: direction changed to LEFT
[2025-09-28T12:31:05] USER_ACTION [root:ADMIN] STATUS: requested vehicle status
```

### **3. Telemetry Broadcasts:**
```
[2025-09-28T12:31:10] Broadcast DATA: DATA 75.5 85 FORWARD 28.3
```

##  Security Features

- ** Password Hashing**: SHA-256 + unique salt per user (no plain text storage)
- ** Login Attempt Limiting**: Account locked after 3 failed attempts for 5 minutes
- ** Role-based Access Control**: Different permissions for ADMIN vs OBSERVER
- ** Secure Authentication**: Hash-based credential validation
- ** Connection Tracking**: All connections and disconnections logged
- ** Action Auditing**: Every user action is logged with timestamp and details
- ** Salt Protection**: Unique salt prevents rainbow table attacks

## 🛠️ Development Tools

### **Password Hash Generator**
Located in `tools/hash_password.py`:

```bash
# Generate hash for individual user
python3 tools/hash_password.py --user username --role ADMIN --password 'password'

# Generate example configuration with default users
python3 tools/hash_password.py --example > config.json.example

# Show JSON output only
python3 tools/hash_password.py --user test --role OBSERVER --password 'secret' --json
```

### **Configuration Management**
- **`config.json`**: Active server configuration (contains real hashes)
- **`config.json.example`**: Template with example users (safe to commit)
- **`tools/hash_password.py`**: Utility for generating secure password hashes

### **Security Best Practices**
1. **Never commit `config.json`** with real hashes to version control
2. **Use `config.json.example`** as template for deployment
3. **Generate unique salts** for each user
4. **Use strong passwords** in production
5. **Rotate credentials** regularly

##  **Quick Start Guide**

### **1. Setup & Build**
```bash
# Install dependencies
sudo apt install build-essential libssl-dev

# Generate example configuration
python3 tools/hash_password.py --example > config.json.example

# Build the server
make clean && make
```

### **2. Run Server**
```bash
# Start server
./server 5050 log.txt

# Test connection
nc 127.0.0.1 5050
```

### **3. Authentication**
```bash
# Login as ADMIN
LOGIN ADMIN root password

# Login as OBSERVER  
LOGIN OBSERVER juan 12345

# Test commands
STATUS
SPEED UP
```

##  **Project Structure**
```
├── server.c              # Main server implementation
├── client_handler.c       # Client connection handling
├── config.c              # User authentication with hashing
├── logger.c              # Logging system
├── tools/
│   └── hash_password.py  # Password hash generator
├── clients/
│   ├── java
│   └── python
├── config.json.example   # Example configuration
└── README.md             # This file
```

##  **Development Workflow**

### **Adding New Users**
```bash
# Generate hash for new user
python3 tools/hash_password.py --user newuser --role ADMIN --password 'newpass' --json

# Add to config.json (never commit this file)
# Copy the generated JSON to your config.json
```

### **Testing Authentication**
```bash
# Test with netcat
nc 127.0.0.1 5050
LOGIN ADMIN root password
STATUS
```

### **Viewing Logs**
```bash
# Real-time logs
tail -f log.txt

# Filter authentication events
tail -f log.txt | grep -E "(LOGIN|USER_ACTION)"
```
