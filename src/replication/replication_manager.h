#pragma once
#include "../handler/event_handler.h"
#include <arpa/inet.h>
#include <string>


enum class ReplicationManagerErrors{
    NO_ERROR,
    MASTER_SOCKET_CRTEATION_ERROR,
    SERVER_SOCKOPT_BINDING_ERROR,
    MASTER_PORT_BINDING_ERROR,
};

enum class HandshakeState{
    DISCONNECTED,
    AWAITING_PONG,
    AWAITING_REPLCONF_PORT_OK,
    AWAITING_REPLCONF_CAPA_OK,
    READY_FOR_PSYNC,
    CONNECTED
};

class ReplicationManager: public IEventHandler{
    private:
    std::string master_host;
    int master_fd;
    int master_port;
    int replica_port;
    std::string master_replid;
    long long master_repl_offset;
    ReplicationManagerErrors error;
    sockaddr_in master_address;
    HandshakeState current_state;

    void sendPing();
    void sendRepliconfPort();
    void SendRepliconfCapa();
    void handleHandshakeFailure(const std::string& fail_msg);

    public:
    ReplicationManager(std::string host, int port, int rep_port);
    int connectToMaster();
    bool startHandshake();
    void processMasterData(const std::string& data);
    virtual void handleEvent(int fd, unsigned int events) override;
};