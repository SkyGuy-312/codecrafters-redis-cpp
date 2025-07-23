#include <iostream>
#include "RedisServer.h"

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  RedisServer server(6379);

  if(server.initialize()){
    server.run();
  }
  
  return 0;
}
