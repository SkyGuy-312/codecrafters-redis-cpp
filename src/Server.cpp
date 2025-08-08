#include <iostream>
#include "config/server_config.h"
#include "RedisServer.h"

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  ServerConfig server_config(argc, argv);

  RedisServer server(server_config.port, server_config);

  if(server.initialize()){
    server.run();
  }
  else{
    std::cerr << "Failed to initialize server" << std::endl;
  }
  
  return 0;
}
