#include "replication_manager.h"
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>

void ReplicationManager::sendPing()
{
    std::string ping = "*1\r\n$4\r\nPING\r\n";
    if (send(this->master_fd, ping.c_str(), ping.length(), 0) < 0){
        this->handleHandshakeFailure("Sending ping to master failed! aborting...");
    }
    this->current_state = HandshakeState::AWAITING_PONG;
}

void ReplicationManager::sendRepliconfPort()
{
    std::string repliconf = 
    "*3\r\n"
    "$8\r\nREPLCONF\r\n"
    "$14\r\nlistening-port\r\n"
    "$" + std::to_string(std::to_string(this->replica_port).length()) + "\r\n" + std::to_string(this->replica_port) +"\r\n";
    if (send(this->master_fd, repliconf.c_str(), repliconf.length(), 0) < 0){
        this->handleHandshakeFailure("Sending repliconf port to master failed! aborting...");
    }
    this->current_state = HandshakeState::AWAITING_REPLCONF_PORT_OK;
}

void ReplicationManager::SendRepliconfCapa()
{
    std::string repliconf = 
        "*3\r\n"
        "$8\r\nREPLCONF\r\n"
        "$4\r\ncapa\r\n"
        "$6\r\npsync2\r\n";
    if (send(this->master_fd, repliconf.c_str(), repliconf.length(), 0) < 0){
        this->handleHandshakeFailure("Sending repliconf capabilities to master failed! aborting...");
    }
    this->current_state = HandshakeState::AWAITING_REPLCONF_CAPA_OK;
}

void ReplicationManager::sendPsync()
{
    std::string psync = 
    "*3\r\n"
    "$5\r\nPSYNC\r\n"
    "$1\r\n?\r\n"
    "$2\r\n-1\r\n";
    if (send(this->master_fd, psync.c_str(), psync.length(), 0) < 0){
        this->handleHandshakeFailure("Sending psync to master failed! aborting...");
    }
    this->current_state = HandshakeState::AWAITING_PSYNC_RESPONSE;
}

void ReplicationManager::handleHandshakeFailure(const std::string& fail_msg)
{
    std::cerr << fail_msg << std::endl;
    close(this->master_fd);
}

ReplicationManager::ReplicationManager(std::string host, int port, int rep_port):
master_host(host),
master_port(port),
replica_port(rep_port),
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
    std::string host_to_use = this->master_host;
    if (host_to_use == "localhost") {
        host_to_use = "127.0.0.1";
    }
    if (inet_pton(AF_INET, host_to_use.c_str(), &this->master_address.sin_addr) <= 0) {
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
    switch(this->current_state){
        case HandshakeState::AWAITING_PONG:
            if (data == "+PONG\r\n"){
                std::cout <<"Received PONG from master. Starting REPLICONF for port.." << std::endl;
                this->sendRepliconfPort();
            }
            else{
                this->handleHandshakeFailure("Expected +PONG, but received: " + data + "\n");
            }
            break;
        case HandshakeState::AWAITING_REPLCONF_PORT_OK:
            if (data == "+OK\r\n"){
                std::cout <<"Received +OK from master. Starting REPLICONF for capabilities..." << std::endl;
                this->SendRepliconfCapa();
            }
            else{
                this->handleHandshakeFailure("Expected +OK, but received: " + data + "\n");
            }
            break;
        case HandshakeState::AWAITING_REPLCONF_CAPA_OK:
            if (data == "+OK\r\n"){
                std::cout <<"Received +OK from master. Starting REPLICONF for capabilities..." << std::endl;
                this->sendPsync();
            }
            else{
                this->handleHandshakeFailure("Expected +OK, but received: " + data + "\n");
            }
            break;

    }


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
