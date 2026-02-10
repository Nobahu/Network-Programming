#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <cstring>

const unsigned int SERVER_PORT = 10001;
const unsigned int BUFFER_SIZE = 2048;

int main() {
  
  int client_fd = socket(AF_INET6,SOCK_STREAM,0);
  if (client_fd < 0) {
    std::cout << "Socket create error" << '\n';
    return 1;
  }

  struct sockaddr_in6 server_addr;
  memset(&server_addr,0,sizeof(server_addr));
  server_addr.sin6_port = htons(SERVER_PORT);
  server_addr.sin6_family = AF_INET6;
  inet_pton(AF_INET6,"::1",&server_addr.sin6_addr);

  if(connect(client_fd,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0) {
    std::cout << "Connection failed!" << '\n';
    close(client_fd);
    return 2;
  }

  std::cout << "Successful connection with server!" << '\n';

  bool isRunning = 1;
  std::string client_message;
  while(isRunning) {

    char buffer[BUFFER_SIZE];
    std::cout << "Message: ";
    std::getline(std::cin,client_message);
    client_message += '\n';

    if(client_message == "exit\n") {
      close(client_fd);
      isRunning = 0;
      break;
    }

    send(client_fd,client_message.c_str(),client_message.length(),0);
    
    int bytes = recv(client_fd,&buffer,sizeof(buffer),0);
    if (bytes > 0) {
      buffer[bytes] = '\0';
      std::cout << "Message from server: " << buffer << '\n';
    }

  }
  close(client_fd);

  return 0;
}