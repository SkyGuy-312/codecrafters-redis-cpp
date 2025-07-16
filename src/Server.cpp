#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <vector>

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  std::cout << "Waiting for a client to connect...\n";

  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    std::cerr << "Failed to create epoll instance\n";
    close(server_fd);
    return 1;
  }

  epoll_event event;
  event.data.fd = server_fd;
  event.events = EPOLLIN;

  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

  constexpr int EVENTS_BUFFER_SIZE {10};
  epoll_event events[EVENTS_BUFFER_SIZE];

  constexpr int DATA_BUFFER_SIZE{1024};
  char buffer [DATA_BUFFER_SIZE];

  std::string PONG {"+PONG\r\n"};
  
  while (true){
    int event_nums = epoll_wait(epoll_fd, events, EVENTS_BUFFER_SIZE, -1);
    switch (event_nums){
      case 0:
        std::cerr << "timeout occrred\n"; break;
      case -1:
        std::cerr << "error occurred\n"; break;
      default:
        // loop through the events:
        for (int i {0}; i < event_nums; i++){
          if (events[i].data.fd == server_fd){
            auto client_fd = accept(server_fd, reinterpret_cast<sockaddr*> (&client_addr), reinterpret_cast<socklen_t*>(&client_addr_len));
            switch (client_fd){
              case -1:
                std::cerr << "client wasn't accepted\n"; break;
              default:
                epoll_event client_event;
                client_event.data.fd = client_fd;
                client_event.events = EPOLLIN;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);
                break;
            }
          }else{
            int num_bytes = recv(events[i].data.fd, buffer, DATA_BUFFER_SIZE, 0);
            switch (num_bytes){
              case -1:
                std::cerr << "error occurred while recieving data from client\n";break;
              case 0:
                std::cerr << "client disconnected\n"; 
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
                close(events[i].data.fd); break;
              default:
                bool abort_flag {};
                // start RESP parsing:
                // 1. convert byte buffer to string
                std::string data(buffer, num_bytes);
                if (data[0] != '*'){
                  std::cerr << "cannot find size of input, aborting...\n";
                  break;
                }else{
                  //find position of \r\n:
                  auto pos = data.find("\r\n");
                  // read the string until that pos:
                  auto length = data.substr(0, pos);
                  // create tokens array from it
                  std::cout << std::stoi(length.substr(1)) << "\n";
                  // std::vector<std::string> tokens(std::stoi(length.substr(1)));  
                  //print elements for debugging:

                  
                  
                }
                

            }
          }
        }
    }
  }  

  close(server_fd);

  return 0;
}
