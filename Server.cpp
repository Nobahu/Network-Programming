#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <ctime>
#include <chrono>
#include <iostream>
#include <cstring>

int main()
{
    char buffer[1024];
    int server_fd;

    server_fd = socket(AF_INET6,SOCK_STREAM,0);
    if(server_fd < 0) {
        std::cout << "Socket create failed" << '\n';
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in6 server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(5050);
    server_addr.sin6_addr = in6addr_any;

    if(bind(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0) {
        std::cout << "Bind failed" << '\n';
        close(server_fd);
        return 2;
    }

    if(listen(server_fd,5) < 0) {
        std::cout << "Server socket listening failed" << '\n';
        close(server_fd);
        return 3;
    }

    std::cout << "Server is listening on port 5050" << '\n';

    int recv_bytes;
    while(true) {
        struct sockaddr_in6 client_addr;
        socklen_t client_addr_length = sizeof(client_addr);
        int client_fd = accept(server_fd,(struct sockaddr*)&client_addr, &client_addr_length);
        if(client_fd < 0) {
            std::cout << "Client_fd create failed" << '\n';
            continue;
        }

        unsigned short client_port = ntohs(client_addr.sin6_port);
        std::cout << "New client connected:\n";
        std::cout << "Client port: " << client_port << "\n";
        std::cout << "Client socket_fd: " << client_fd << "\n";

        recv_bytes = 0;
        recv_bytes = recv(client_fd,buffer,sizeof(buffer) - 1,0);
        if(recv_bytes > 0) {
            buffer[recv_bytes] = '\0';
            std::cout << "Message from client: " << buffer << '\n';
        }
        auto now = std::chrono::system_clock::now();
        std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
        send(client_fd,&time_t_now,sizeof(time_t_now),0);
        
        close(client_fd);

        std::cout << "================================================" << '\n';
    }

    close(server_fd);
    return 0;
}