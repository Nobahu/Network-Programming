#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "ThreadPool.h"

#include <mutex>
#include <thread>

#include <iostream>
#include <vector>
#include <cstring>

class MtServer
{
public:

  MtServer(): server_fd(-1), isRunning(false) 
  {
    if(!socketSetup()) {
      throw std::runtime_error("Failed to setup socket");
    }
    std::cout << "Server started on port " << SERVER_PORT << std::endl;
  }

  ~MtServer()
  {
    close(server_fd);
  }

  void run()
  {
    fd_set read_fds;
    isRunning = 1;
    while(isRunning) {
      int max_fd = createFDSet(read_fds);

      struct timeval timeout{2,0};
      int ready_socket = select(max_fd + 1,&read_fds,NULL,NULL,&timeout);
      if(ready_socket < 0) {
        if(errno == EINTR) continue;
        std::cout << "Select error" << '\n';
        break;
      }
      if(ready_socket == 0) {
        continue;
      }

      handleNewConnection(read_fds);
      handleClient(read_fds);
    }
    
  }

private:

  bool socketSetup()
  {
    server_fd = socket(AF_INET6,SOCK_STREAM,0);
    if (server_fd < 0) {
      std::cout << "Socket create error" << '\n';
      return false;
    }

    struct sockaddr_in6 server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin6_port = htons(SERVER_PORT);
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;

    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    if(bind(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0) {
      std::cout << "Socket bind failed!" << '\n';
      close(server_fd);
      return false;
    }

    if(listen(server_fd,5) < 0) {
      std::cout << "Listen failed!" << '\n';
      close(server_fd);
      return false;
    }
    return true;
  }

  int createFDSet(fd_set& read_fds)
  {
    FD_ZERO(&read_fds);
    FD_SET(server_fd,&read_fds);
    int max_fd = server_fd;

    for(int client: client_sockets) {
      FD_SET(client,&read_fds);
      if(client > max_fd) {
        max_fd = client;
      }
    }
    return max_fd;
  }

  void handleNewConnection(const fd_set& read_fds)
  {
    if(FD_ISSET(server_fd,&read_fds)) {
      struct sockaddr_in6 client_addr;
      socklen_t client_addr_len = sizeof(client_addr);

      int client_fd = accept(server_fd,(struct sockaddr*)&client_addr,&client_addr_len);
      if(client_fd < 0) {
        std::cout << "Acception failed" << '\n';
        return;
      }
      unsigned int client_port = ntohs(client_addr.sin6_port);
      std::cout << "New client connected:\n";
      std::cout << "Client port: " << client_port << "\n";
      std::cout << "Client socket_fd: " << client_fd << "\n";

      client_sockets.push_back(client_fd);
    }
  }

  void handleClient(const fd_set& read_fds)
  {
    for(auto it = client_sockets.begin();it < client_sockets.end();) {

      int client_fd = *it;
      char buffer[BUFFER_SIZE];
      if(FD_ISSET(client_fd,&read_fds)) {
        int bytes = recv(client_fd,&buffer,sizeof(buffer),0);
        if(bytes > 0) {
          buffer[bytes] = '\0';
          thread_poll.add_task([this,client_fd,data = std::string(buffer,bytes)]() {
            std::cout << "Client (" << client_fd << "): " << data << '\n';
            std::lock_guard<std::mutex> lock(mtx);
            send(client_fd,server_message.data(),server_message.size(),0);
          });
        }
        else {
          std::cout << "Client (" << client_fd << "): disconnected" << '\n';
          close(client_fd);
          it = client_sockets.erase(it);
          continue;
        }
      }
      it++;
    }
  }



  const unsigned int SERVER_PORT = 10001;
  const unsigned int BUFFER_SIZE = 2048;

  int opt = 1;
  int server_fd;
  bool isRunning;

  std::vector<int> client_sockets;
  std::string server_message = "////////// Message received by server //////////";
  ThreadPool thread_poll;
  std::mutex mtx;
};