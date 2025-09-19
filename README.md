# Redis Clone Implementation in C++

[![progress-banner](https://backend.codecrafters.io/progress/redis/dbfbc7cc-6189-46f7-9999-c5a05f3f6c3d)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

A high-performance Redis clone implementation in C++ featuring a modular, event-driven architecture. This project implements core Redis functionality including basic commands, persistence, and master-replica replication.

## 🚀 Features

### Core Commands
- **PING** - Server connectivity test
- **SET/GET** - Key-value storage with optional expiration (PX/EX)
- **ECHO** - Echo back messages
- **KEYS** - Pattern-based key listing
- **INFO** - Server information (replication status)
- **CONFIG** - Runtime configuration access

### Advanced Features
- **RDB Persistence** - Redis Database file format support
- **Master-Replica Replication** - Full replication handshake and command propagation
- **Command Propagation** - Automatic write command forwarding to replicas
- **Event-Driven Architecture** - High-performance epoll-based event loop
- **Modular Design** - Clean separation of concerns with dependency injection

## 🏗️ Architecture

### Core Components

#### EventLoop (`src/events/`)
```cpp
class EventLoop {
    // Generic epoll-based event dispatcher
    // Manages file descriptors and delegates events to handlers
    bool addFd(int fd, uint32_t events, IEventHandler* handler);
    void run();  // Main event loop
};
```

#### RedisServer (`src/RedisServer.h`)
```cpp
class RedisServer : public IEventHandler {
    // Main server class managing client connections
    // Handles command execution and replica management
    bool initialize();
    void handleEvent(int fd, uint32_t events) override;
    bool startReplication();  // For replica mode
};
```

#### CommandHandler (`src/commands/`)
```cpp
class CommandHandler {
    // Command pattern implementation
    // Manages command registry and execution
    std::string executeCommand(std::vector<std::string>& tokens);
    bool isWriteCommand(std::string& cmd);
};
```

#### ReplicationManager (`src/replication/`)
```cpp
class ReplicationManager : public IEventHandler {
    // Handles master-replica communication
    // Manages handshake state machine and command processing
    int connectToMaster();
    bool startHandshake();
    void handleEvent(int fd, uint32_t events) override;
};
```

### Data Flow

1. **Client Connection**: EventLoop detects new connections on server socket
2. **Command Processing**: RedisServer receives data, parses RESP, executes commands
3. **Write Propagation**: Write commands are automatically forwarded to all connected replicas
4. **Replication**: Replicas connect via handshake (PING → REPLCONF → PSYNC) and receive command stream

## 🛠️ Building & Running

### Prerequisites
- CMake 3.13+
- C++23 compatible compiler (GCC 10+, Clang 12+)
- vcpkg package manager

### Build Instructions
```bash
# Clone the repository
git clone https://git.codecrafters.io/9593de7f7d1e19a0
cd codecrafters-redis-cpp

# Build the project
./your_program.sh

# Or manually:
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
cmake --build ./build
```

### Running the Server
```bash
# Start as master (default port 6379)
./build/server

# Start with custom port
./build/server --port 6380

# Start as replica
./build/server --port 6380 --replicaof localhost 6379

# Load from RDB file
./build/server --dir /path/to/data --dbfilename dump.rdb
```

## 📁 Project Structure

```
src/
├── Server.cpp              # Main entry point
├── RedisServer.{h,cpp}      # Core server implementation
├── commands/                # Command pattern implementations
│   ├── command.h           # Base command interface
│   ├── command_handler.{h,cpp}
│   ├── ping_command.{h,cpp}
│   ├── set_command.{h,cpp}
│   ├── get_command.{h,cpp}
│   ├── info_command.{h,cpp}
│   ├── config_command.{h,cpp}
│   ├── replconf_command.{h,cpp}
│   └── psync_command.{h,cpp}
├── config/                  # Configuration management
│   ├── server_config.{h,cpp}
├── events/                  # Event loop implementation
│   └── event_loop.{h,cpp}
├── handler/                 # Event handler interface
│   └── event_handler.h
├── replication/            # Replication logic
│   └── replication_manager.{h,cpp}
├── resp/                   # Redis Serialization Protocol
│   └── parser.{h,cpp}
├── rdb/                    # Redis Database persistence
│   └── rdb_parser.{h,cpp}
└── storage/                # In-memory storage
    └── in_memory_store.{h,cpp}
```

## 🔧 Configuration Options

### Command Line Arguments
- `--port <port>` - Server listening port (default: 6379)
- `--dir <directory>` - RDB file directory
- `--dbfilename <filename>` - RDB filename
- `--replicaof <host> <port>` - Run as replica of specified master

### Example Configurations
```bash
# Master server with persistence
./build/server --port 6379 --dir ./data --dbfilename dump.rdb

# Replica server
./build/server --port 6380 --replicaof localhost 6379
```

## 🧪 Testing

### Manual Testing
```bash
# Test basic commands
redis-cli -p 6379 PING
redis-cli -p 6379 SET mykey "Hello World"
redis-cli -p 6379 GET mykey

# Test replication
redis-cli -p 6379 SET key1 value1  # On master
redis-cli -p 6380 GET key1         # Should return value1 on replica
```

### Supported RESP Commands
| Command | Syntax | Description |
|---------|--------|-------------|
| PING | `PING` | Test server connectivity |
| SET | `SET key value [PX ms] [EX s]` | Set key-value with optional expiration |
| GET | `GET key` | Retrieve value by key |
| ECHO | `ECHO message` | Echo back the message |
| KEYS | `KEYS pattern` | List keys matching pattern (* for all) |
| INFO | `INFO REPLICATION` | Get replication information |
| CONFIG | `CONFIG GET parameter` | Get configuration parameter |

## 🎯 Design Patterns

### Command Pattern
Each Redis command is implemented as a separate class inheriting from `Command`:
```cpp
class SetCommand : public Command {
    std::string execute(const std::vector<std::string>& tokens, 
                       InMemoryStore& storage, 
                       const ServerConfig& config) override;
    bool isWriteCmd() const override { return true; }
};
```

### Observer Pattern
Event-driven architecture using `IEventHandler` interface:
```cpp
class IEventHandler {
    virtual void handleEvent(int fd, unsigned int events) = 0;
};
```

### Strategy Pattern
Configurable server behavior based on role (master/replica).

## 🔄 Replication Protocol

### Handshake Sequence
1. **PING** → **+PONG**
2. **REPLCONF listening-port \<port>** → **+OK**
3. **REPLCONF capa psync2** → **+OK**
4. **PSYNC ? -1** → **+FULLRESYNC \<replid> \<offset>**
5. **RDB Transfer** (binary data)
6. **Command Stream** (continuous RESP commands)

### State Machine
```
DISCONNECTED → AWAITING_PONG → AWAITING_REPLCONF_PORT_OK 
→ AWAITING_REPLCONF_CAPA_OK → AWAITING_PSYNC_RESPONSE 
→ RECEIVING_RDB_FILE → CONNECTED
```

## 🚀 Performance Features

- **Zero-copy networking** where possible
- **Event-driven I/O** with Linux epoll
- **Non-blocking sockets** for high concurrency
- **Efficient memory management** with smart pointers
- **Minimal string copying** in RESP parsing

## 🔮 Future Enhancements

- [ ] Pub/Sub functionality
- [ ] Clustering support
- [ ] Additional data types (Lists, Sets, Sorted Sets)
- [ ] Streams data type

## 📄 License

This project is part of the [CodeCrafters Redis Challenge](https://codecrafters.io/challenges/redis).

## 🤝 Contributing

This is an educational project. Feel free to explore the code and suggest improvements!

---

**Note**: This implementation focuses on educational value and architectural clarity rather than production-level performance and feature completeness.