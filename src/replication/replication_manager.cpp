#include "replication_manager.h"
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>

void ReplicationManager::sendPing()
{
    std::string ping = "*1\r\n$4\r\nPING\r\n";
    if (send(this->master_fd, ping.c_str(), ping.length(), 0) < 0){
        std::cerr << "Sending ping failed to master! aborting..." << std::endl;
    }
}

ReplicationManager::ReplicationManager(std::string host, int port):
master_host(host),
master_port(port),
error(ReplicationManagerErrors::NO_ERROR)
{
}

int ReplicationManager::connectToMaster()
{
    this->master_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->master_fd < 0){
        std::cerr << "Failed to create master socket\n";
        this->error = ReplicationManagerErrors::MASTER_SOCKET_CRTEATION_ERROR;
        return -1;
    }
    // Since the tester restarts the program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(this->master_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
        std::cerr << "setsockopt failed\n";
        this->error = ReplicationManagerErrors::SERVER_SOCKOPT_BINDING_ERROR;
        close(this->master_fd);
        return -1;
    }

    
    this->master_address.sin_family = AF_INET;
    this->master_address.sin_port = htons(this->master_port);
    if (inet_pton(AF_INET, this->master_host.c_str(), &this->master_address.sin_addr) <= 0) {
        std::cerr << "Invalid master host address\n";
        this->error = ReplicationManagerErrors::MASTER_SOCKET_CRTEATION_ERROR;
        close(this->master_fd);
        return -1;
    }

    if (connect(this->master_fd, reinterpret_cast<const sockaddr*>(&this->master_address), sizeof(this->master_address)) < 0) {
        std::cerr << "Failed to connect to master\n";
        this->error = ReplicationManagerErrors::MASTER_SOCKET_CRTEATION_ERROR;
        close(this->master_fd);
        return -1;
    }

    return this->master_fd;
}

bool ReplicationManager::startHandshake()
{
    sendPing();
    return true;
}

void ReplicationManager::processMasterData(const std::string &data)
{
    if (data.find("+PONG") != std::string::npos) std::cout <<"Received PONG from master." << std::endl;
}

void ReplicationManager::handleEvent(int fd, unsigned int events)
{
    char buffer[1024];
    int bytes_received = recv(fd, buffer, sizeof(buffer)-1, 0);
    if (bytes_received < 0){
        std::cerr << "Master Disconnected" << std::endl;
        close(fd);
    }
    buffer[bytes_received] = '\0';
    this->processMasterData(std::string(buffer));
}
