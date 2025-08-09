#pragma once
#include "storage/in_memory_store.h"
#include "commands/command_handler.h"
#include "config/server_config.h"
#include "rdb/rdb_parser.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <string>
#include <vector>

enum class RedisServerSocketErrors{
    server_no_error,
    server_socket_creation_error,
    server_sockopt_binding_error,
    server_port_binding_error,
    server_listen_error,
};

enum class RedisServerEpollErrors{
    epoll_no_error,
    epoll_fd_creation_error,
    epoll_ctl_error,
};

enum class RedisServerClientErrors{
    client_no_error,
    client_accept_error,
};


class RedisServer{
    private:
    static constexpr int DATA_BUFFER_SIZE {1000};
    static constexpr int TRIGGERED_EVENTS_BUFFER_SIZE {10};
    static constexpr int EPOLL_CREATE1_NOFLAGS = 0;

    int server_fd{};
    int epoll_fd{};

    InMemoryStore storage;
    CommandHandler command_handler;
    RDBParser rdb_parser;

    sockaddr_in server_address{};    
    epoll_event server_event{};
    int server_port{};
    
    
    epoll_event triggered_events_buffer[TRIGGERED_EVENTS_BUFFER_SIZE];
    char data_buffer[DATA_BUFFER_SIZE];

    void handleNewConneciton(sockaddr_in& new_conn_addr,  socklen_t& new_conn_len);
    void handleClientData(int client_fd);
    bool sendResponse (int client_fd, const std::string& response_string);

    public:

    RedisServerSocketErrors socket_error {};
    RedisServerEpollErrors epoll_error{};
    RedisServerClientErrors client_error{};

    // Disallow copying - moving since commands are unique ptrs
    /* EXPLAINATION:
        Commands are put in a map which have the commands as uniqure ptrs, this is in the command handler class.
        This makes the commmand handler class non-copyable.
        The Command handler is composite as an object within the RedisServer class, by turn, 
        this must mark the Redis server class as also non-copyable, however it is not explicit, implictily compiler will try and generate copy/move constructors
        so we delete them.
    */
    RedisServer(int port_number,const ServerConfig& rdb_fd);
    RedisServer(const RedisServer&)=delete;
    RedisServer& operator=(const RedisServer&)=delete;
    ~RedisServer();
    bool initialize();
    bool run ();
    
    


};