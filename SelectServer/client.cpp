#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <cstring>


int main()

{
  //File descriptor creation
  int client_fd = socket(AF_INET6,SOCK_STREAM,0);
  if(client_fd < 0) {
      std::cout << "Socket create failed" << '\n';
      return 1;
  }

  // Server address creation with 5050 port
  struct sockaddr_in6 server_addr;
  memset(&server_addr,0,sizeof(server_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(5050);
  inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);

  if(connect(client_fd,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0) {
    std::cout << "Connection error" << '\n';
    close(client_fd);
    return 1;
  }
  std::cout << "Successful connection with server!" << '\n';

  while(true) {
    std::cout << "Message: ";
    std::string message;
    std::getline(std::cin, message);

    if(message == "exit") {
      break;
    }

    send(client_fd,message.c_str(),message.length(),0);

    char buffer[1024];
    int bytes = recv(client_fd,buffer,sizeof(buffer),0);
    if (bytes > 0) {
      std::cout << "Message from server: " << buffer << '\n';
    }
  }

  close(client_fd);
  return 0;
}