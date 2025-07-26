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

RedisServer::RedisServer(int port_number,const ServerConfig& rdb_fd):command_handler(this->storage, rdb_fd), rdb_parser(this->storage)
{
    using namespace std::filesystem;
    this->server_port = port_number;
    this->server_fd = -1;
    this->epoll_fd = -1;
    this->socket_error = RedisServerSocketErrors::server_no_error;
    this->epoll_error = RedisServerEpollErrors::epoll_no_error;
    this->client_error = RedisServerClientErrors::client_no_error;

    path full_path = path(rdb_fd.dir)/rdb_fd.dbfilename;
    std::cout << full_path.string() << std::endl;
    rdb_parser.parse(full_path.string());

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
    // Since the tester restarts your program quite often, setting SO_REUSEADDR
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

    this->epoll_fd = epoll_create1(this->EPOLL_CREATE1_NOFLAGS);
    if (this->epoll_fd < 0)
    {
        std::cerr << "Failed to create epoll instance\n";
        this->epoll_error = RedisServerEpollErrors::epoll_fd_creation_error;
        switch (errno)
        {
        case EMFILE:
            std::cerr << "epoll fd creation failed: too many open file descriptors! program resource limit reachaed." << std::endl;
            close(this->server_fd);
            break;
        case ENFILE:
            std::cerr << "epoll fd creation failed: system-wide resource limit for file descriptor reached." << std::endl;
            close(this->server_fd);
            break;
        case EINVAL:
            std::cerr << "Invalid flag argument.." << std::endl;
            close(this->server_fd);
            break;
        }

        return false;
    }

    this->server_event.data.fd = server_fd;
    this->server_event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &this->server_event) < 0)
    {
        this->epoll_error = RedisServerEpollErrors::epoll_ctl_error;
        switch (errno)
        {
        case EBADF:
            std::cerr << "server epoll_ctl error: invalid file descriptor" << std::endl;
            close(this->server_fd);
            close(this->epoll_fd);
            break;
        case EEXIST:
            std::cerr << "server epoll_ctl error: Server FD already being monitored" << std::endl;
            break;
        case ENOMEM:
            std::cerr << "server epoll_ctl error: kernel out of memory" << std::endl;
            close(this->server_fd);
            close(this->epoll_fd);
            break;
        default:
            perror("server epoll_ctl failed with unexpected error");
            close(this->server_fd);
            close(this->epoll_fd);
            break;
        }
    }


    return true;
}

bool RedisServer::run()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    std::cout << "Waiting for a client to connect...\n";

    while (true)
    {
        int event_nums = epoll_wait(this->epoll_fd, this->triggered_events_buffer, TRIGGERED_EVENTS_BUFFER_SIZE, -1);
        switch (event_nums)
        {
        case 0:
            std::cerr << "timeout occrred\n";
            break;
        case -1:
            std::cerr << "error occurred\n";
            break;
        default:
            for (int i{0}; i < event_nums; i++)
            {
                if (triggered_events_buffer[i].data.fd == server_fd)
                {
                    this->handleNewConneciton(client_addr, client_addr_len);
                }
                else
                {
                    this->handleClientData(triggered_events_buffer[i].data.fd);
                }
            }
            break;
        }
    }
}

void RedisServer::handleNewConneciton(sockaddr_in &new_conn_addr, socklen_t &new_conn_len)
{
    auto client_fd = accept(this->server_fd, reinterpret_cast<sockaddr *>(&new_conn_addr), &new_conn_len);
    switch (client_fd)
    {
    case -1:
        std::cerr << "client wasn't accepted" << std::endl;
        this->client_error = RedisServerClientErrors::client_accept_error;
        break;
    default:
        epoll_event client_event;
        client_event.data.fd = client_fd;
        client_event.events = EPOLLIN;
        if (epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) < 0)
        {
            switch (errno)
            {
            case EBADF:
                std::cerr << "client epoll_ctl error: invalid file descriptor" << std::endl;
                close(client_fd);
                break;
            case EEXIST:
                std::cerr << "client epoll_ctl error: client FD already being monitored" << std::endl;
                break;
            case ENOMEM:
                std::cerr << "client epoll_ctl error: kernel out of memory" << std::endl;
                close(client_fd);
                break;
            default:
                perror("client epoll_ctl failed with unexpected error");
                close(client_fd);
                break;
            }
        }
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
        epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
        close(client_fd);
        break;
    case 0:
        std::cerr << "client gracefully disconnected" << std::endl;
        epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
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
            epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
            close(client_fd);
            return false;
        case ECONNRESET:
            std::cerr << "Client forcefully disconnected" << std::endl;
            epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
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
            epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
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
    if (this->epoll_fd != -1)
    {
        close(this->epoll_fd);
    }
}