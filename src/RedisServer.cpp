#include "RedisServer.h"
#include "resp/parser.h"
#include "commands/command_handler.h"
#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <algorithm>
#include <unordered_map>
#include "rdb/rdb_parser.h"
#include <filesystem>

RedisServer::RedisServer(EventLoop& loop, int port_number,const ServerConfig& rdb_fd):
command_handler(this->storage, rdb_fd),
rdb_parser(this->storage),
replication_manager("", 0, 0),
server_fd(-1),
server_port(port_number),
socket_error(RedisServerSocketErrors::server_no_error),
epoll_error(RedisServerEpollErrors::epoll_no_error),
client_error(RedisServerClientErrors::client_no_error),
event_loop(loop)
{
    using namespace std::filesystem;

    if (!rdb_fd.dir.empty() && !rdb_fd.dbfilename.empty()) {
        path full_path = path(rdb_fd.dir)/rdb_fd.dbfilename;
        std::cout << full_path.string() << std::endl;
        rdb_parser.parse(full_path.string());
    }
    if (rdb_fd.replication_info.role == "slave"){
        replication_manager= ReplicationManager(rdb_fd.replication_info.master_host, rdb_fd.replication_info.master_port, this->server_port);
    }
}

bool RedisServer::initialize()
{
    this->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "Failed to create server socket\n";
        this->socket_error = RedisServerSocketErrors::server_socket_creation_error;
        return false;
    }
    // Since the tester restarts the program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(this->server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        std::cerr << "setsockopt failed\n";
        this->socket_error = RedisServerSocketErrors::server_sockopt_binding_error;
        close(this->server_fd);
        return false;
    }

    this->server_address.sin_family = AF_INET;
    this->server_address.sin_addr.s_addr = INADDR_ANY;
    this->server_address.sin_port = htons(this->server_port);

    if (bind(this->server_fd, (struct sockaddr *)&this->server_address, sizeof(this->server_address)) != 0)
    {
        std::cerr << "Failed to bind to port " << this->server_port << std::endl;
        this->socket_error = RedisServerSocketErrors::server_port_binding_error;
        close(this->server_fd);
        return false;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0)
    {
        std::cerr << "listen failed\n";
        this->socket_error = RedisServerSocketErrors::server_listen_error;
        close(this->server_fd);
        return false;
    }

    if (!event_loop.addFd(this->server_fd, EPOLLIN, this)) return false;

    return true;
}

void RedisServer::handleEvent(int fd, uint32_t events)
{
    if (fd == this->server_fd){

        this->handleNewConneciton();
    }
    else{
        this->handleClientData(fd);
    }
}

bool RedisServer::startReplication()
{
    int master_fd = this->replication_manager.connectToMaster();
    if (master_fd < 0){
        std::cerr << "Failed to initiate communication to master" << std::endl;
        return false;
    }
    epoll_event master_event;
    master_event.data.fd = master_fd;
    master_event.events = EPOLLIN;
    event_loop.addFd(master_fd, EPOLLIN, &this->replication_manager);

    replication_manager.startHandshake();
    return true;
}

void RedisServer::handleNewConneciton()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    auto client_fd = accept(this->server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);
    switch (client_fd)
    {
    case -1:
        std::cerr << "client wasn't accepted" << std::endl;
        this->client_error = RedisServerClientErrors::client_accept_error;
        break;
    default:
        event_loop.addFd(client_fd, EPOLLIN, this);
        break;
    }
}

void RedisServer::handleClientData(int client_fd)
{
    int num_bytes = recv(client_fd, this->data_buffer, DATA_BUFFER_SIZE, 0);
    switch (num_bytes)
    {
    case -1:
        std::cerr << "error occurred while recieving data from client" << std::endl;
        event_loop.delFd(client_fd);
        close(client_fd);
        break;
    case 0:
        std::cerr << "client gracefully disconnected" << std::endl;
        event_loop.delFd(client_fd);
        close(client_fd);
        break;
    default:
        std::vector<std::string> tokens = RespParser::dataParser(this->data_buffer, num_bytes);
        if (!tokens.empty())
        {
            auto response = command_handler.executeCommand(tokens);
            this->sendResponse(client_fd, response);
        }
        break;
    }
}


bool RedisServer::sendResponse(int client_fd, const std::string& response_string)
{
    if (send(client_fd, response_string.c_str(), response_string.size(), 0) < 0)
    {
        switch (errno)
        {
        case EPIPE:
            std::cerr << "Client already gracefully disconnected" << std::endl;
            event_loop.delFd(client_fd);
            close(client_fd);
            return false;
        case ECONNRESET:
            std::cerr << "Client forcefully disconnected" << std::endl;
            event_loop.delFd(client_fd);
            close(client_fd);
            return false;
        case EAGAIN: // or EWOULDBLOCK
            // The buffer is full. This is not an error.
            // Log that the send buffer is full.
            // Here you would implement the logic to wait for EPOLLOUT.
            // For now, you can just treat it as a temporary failure.
            return false;
        default:
            perror("Unexpected send error");
            event_loop.delFd(client_fd);
            close(client_fd);
            return false;
        }
    }
    return true;
}

RedisServer::~RedisServer()
{
    if (this->server_fd != -1)
    {
        close(this->server_fd);
    }
    event_loop.delFd(this->server_fd);
}