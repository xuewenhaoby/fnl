#include <stdio.h>  
#include <string.h>   
#include <sys/socket.h>   
#include <arpa/inet.h>  
#include <unistd.h>
#include <iostream>
using namespace std;

#define BUF_SIZE 1024
#define PORT 8000
#define DST_ADDR "100.0.0.255"

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
    if(argc != 2 || strcmp(argv[1],"register"))
        cout << "Wrong input!" << endl;
    else
    {
        int client_fd = 0;
        if((client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            perror("socket error!\n");
            return -1;
        }
        
        char *buf = (char *)"This is a register packet";
        socklen_t len = 0;
        sockaddr_in server_addr = socketAddr(DST_ADDR,PORT);
        
        if((len = sendto(client_fd,buf,strlen(buf),0,(sockaddr*)&server_addr,sizeof(sockaddr))) < 0)
        {
            perror("sendto fail!\n");
            return -1;
        }

        close(client_fd);
    }

    return 0;
}
