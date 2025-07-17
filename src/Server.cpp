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
#include <algorithm>
#include <cctype>

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
                  // 2. create position index, with getting array input size as first position:
                  auto pos = data.find("\r\n");
                  if (pos == std::string::npos){
                    std::cerr << "error parsing upcoming string: invalid string structure\n";
                  }
                  
                  // 3. create tokens array from it
                  std::vector<std::string> tokens(std::stoi(data.substr(1, pos)));  
                  
                  // 4. reset data buffer (we're using it as FIFO)
                  pos += std::string("\r\n").length();
                  data = data.substr(pos);
                  pos = 0;
                  
                  // 5. parse the bulk string
                  while (!data.empty()){
                    //new bulk string, getting size:
                    auto len = data.substr(pos+1, data.find("\r\n"));
                    pos += data.find("\r\n") + std::string("\r\n").length();

                    // get the string itself:
                    auto str = data.substr(pos, std::stoi(len));
                    std::cout << str << "\n";
                    if (str.length() == std::stoi(len)){
                      tokens.push_back(str);
                    }
                    else{
                      std::cerr << "fatal error: input number " << (tokens.size() * 2)-1 << "has invalid length\n";
                    }
                    
                    // delete the read data and reset position:
                    pos = data.find("\r\n", pos) + std::string("\r\n").length();
                    data = data.substr(pos);
                    pos = 0;
                  
                  }

                  // processing commands
                  if (!tokens.empty()){
                    std::string command = tokens[0];
                    
                    std::transform(command.begin(), command.end(), command.begin(), tolower);

                    if (command == "echo" && tokens.size() > 1){
                      // send the command values back to client
                      std::string response{};
                      for (int j = 1; j < tokens.size(); j++){
                        
                        response += "$" + std::to_string(tokens[j].length()) + "\r\n" + tokens[j] + "\r\n";
                      }
                      send(events[i].data.fd, response.c_str(), response.size(), 0);
                    }
                    else if (command == "PING"){
                      send(events[i].data.fd, PONG.c_str(), PONG.length(), 0);
                    }
                  }
                  
                  
                }
                


                

            }
          }
        }
    }
  }  

  close(server_fd);

  return 0;
}
