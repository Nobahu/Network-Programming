#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <ctime>

int main()
{
    char buffer[1024];
    char message[] = "Send me current time,pls";
    int client_fd = socket(AF_INET6,SOCK_STREAM,0);
    if(client_fd < 0) {
        std::cout << "Socket create failed" << '\n';
        return 1;
    }
    struct sockaddr_in6 server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(5050);
    inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);

    if(connect(client_fd,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0) {
        std::cout << "Connection failed" << '\n';
        close(client_fd);
        return 2;
    }

    send(client_fd,message,strlen(message),0);

    int bytes = 0;
    std::time_t received_time;
    bytes = recv(client_fd, &received_time, sizeof(received_time), 0);
    if(bytes == sizeof(received_time)) {
        std::cout << "Time: " << std::ctime(&received_time);
    } 
    else if(bytes > 0) {
        std::cout << "Received incomplete time (" << bytes << " of " 
        << sizeof(received_time) << " bytes)" << '\n';
    }

    close(client_fd);
    return 0;
}
