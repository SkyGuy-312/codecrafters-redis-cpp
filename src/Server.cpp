#include <iostream>
#include "events/event_loop.h"
#include "config/server_config.h"
#include "RedisServer.h"

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  EventLoop event_loop;

  ServerConfig server_config(argc, argv);

  RedisServer server(event_loop, server_config.port, server_config);

  if(!server.initialize()){
    std::cerr << "Failed to initialize server" << std::endl;
    return 1;
  }
  if (server_config.replication_info.role == "slave"){
    server.startReplication();
  }

  event_loop.run();
  
  return 0;
}