#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <iostream>

class SelectServer
{
public:

  SelectServer(int port = 5050): server_fd(-1), running(false),port(port) {
    if (!socketSetup()) {
        throw std::runtime_error("Failed to setup socket");
    }
  }

  ~SelectServer()
  {
    stop();
  }

  void run()
  {
    std::cout << "Server started on port " << port << std::endl;
    running = true;
    fd_set read_fds;

    while(running) {
      int max_fd = createFDSet(read_fds);

      struct timeval timeout{1,0};
      int ready_socket = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
      if(ready_socket < 0) {
        if(errno == EINTR) continue;
        std::cout << "Select error" << '\n';
        break;
      }
      if(ready_socket == 0) {
        continue;
      }

      handleNewConnection(read_fds);
      handleClientData(read_fds);
    }
  }

private:

  /*Create set of client-sockets*/
  int createFDSet(fd_set& read_fds)
  {
    FD_ZERO(&read_fds);
    FD_SET(server_fd,&read_fds);

    int max_fd = server_fd;
    for(int client_fd: client_sockets) {
      FD_SET(client_fd,&read_fds);
      if(client_fd > max_fd) {
        max_fd = client_fd;
      }
    }
    return max_fd;
  }

  void handleNewConnection(fd_set& read_fds)
  {
    if(FD_ISSET(server_fd,&read_fds)) {
      struct sockaddr_in6 client_addr;
      socklen_t client_addr_length = sizeof(client_addr);

      int client_fd = accept(server_fd,(struct sockaddr*)&client_addr,&client_addr_length);
      if(client_fd < 0) {
        std::cout << "Acception failed" << '\n';
      }
      else {
        unsigned short client_port = ntohs(client_addr.sin6_port);
        std::cout << "New client connected:\n";
        std::cout << "Client port: " << client_port << "\n";
        std::cout << "Client socket_fd: " << client_fd << "\n";

        client_sockets.push_back(client_fd);
      }
    }
  }

  /*Send-receive mechanism between server and client*/
  void handleClientData(fd_set& read_fds)
  {
    for(auto it = client_sockets.begin();it != client_sockets.end();) {
      int client_fd = *it;

      if(FD_ISSET(client_fd,&read_fds)) {
        /* Buffer creates here cause of easy way to get round buffer-overflow */
        char buffer[BUFFER_SIZE];

        int bytes = recv(client_fd,buffer,sizeof(buffer),0);
        if(bytes > 0) {
          buffer[bytes] = '\0';
          std::cout << "Client (" << client_fd << "): " << buffer << '\n';
          /*Echo-message*/
          send(client_fd, "///////////////////////////", sizeof("///////////////////////////"), 0);
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

  /*Server socket creation*/
  bool socketSetup()
  {
    server_fd = socket(AF_INET6,SOCK_STREAM,0);
    if(server_fd < 0) {
        std::cout << "Socket create failed" << '\n';
        return false;
    }

    int opt = 1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    struct sockaddr_in6 server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
    server_addr.sin6_addr = in6addr_any;

    if(bind(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0) {
      std::cout << "Bind failed" << '\n';
      close(server_fd);
      return false;
    }
    
    if(listen(server_fd,listen_queue) < 0) {
      std::cout << "Listen failed" << '\n';
      close(server_fd);
      return false;
    }

    return true;
  }

  /*Delete all sockets when we are closing our server*/
  void stop() {
    running = false;
    for (int client_fd : client_sockets) {
      close(client_fd);
    }
    close(server_fd);
    client_sockets.clear();
  }

  const size_t BUFFER_SIZE = 1024;
  const unsigned int listen_queue = 5;

  int server_fd;
  int port;
  bool running = true;
  std::vector<int> client_sockets;
};