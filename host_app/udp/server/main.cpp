#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define PORT 8000

sockaddr_in socketAddr(const char* addrStr,int port)
{
    sockaddr_in sk;
    bzero(&sk, sizeof(sockaddr_in));
    sk.sin_family = AF_INET;
    sk.sin_addr.s_addr = inet_addr(addrStr);
    sk.sin_port = htons(port);
    return sk;
}

int main(int argc, char *argv[])
{
    int server_fd = 0;
    if((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket error!\n");
        return -1;
    }

    sockaddr_in server_addr = socketAddr("0.0.0.0",PORT);
    if(bind(server_fd, (sockaddr *)&server_addr, sizeof(sockaddr)) < 0)
    {
        perror("bind error!\n");
        return -1;
    }

	sockaddr_in client_addr;
    socklen_t len = 0, sin_size = sizeof(sockaddr_in);
    char buf[BUF_SIZE] = {0};
    while(1)
    {
        if((len = recvfrom(server_fd, buf, BUF_SIZE, 0, (sockaddr *)&client_addr, &sin_size)) < 0)
        {
            perror("recvfrom error!\n");
            return -1;
        }

        printf("recv from: %s\n",inet_ntoa(client_addr.sin_addr));
        buf[len] = '\0';
        printf("recv is: %s\n",buf);
    }

    close(server_fd);

    return 0;
}