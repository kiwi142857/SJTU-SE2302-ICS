#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#define PORT 10086
//#define MAX_DATA_SIZE 1400
#define MAX_DATA_SIZE 16384
//#define SERVER_ADDR "202.120.40.85"
#define SERVER_ADDR "127.0.0.1"

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[MAX_DATA_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address / Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    while(1) {
        #if 0
        send(sock, buffer, MAX_DATA_SIZE, 0);
        recv(sock, buffer, MAX_DATA_SIZE, 0);
        #endif
        write(sock, buffer, MAX_DATA_SIZE);
        read(sock, buffer, MAX_DATA_SIZE);
    }

    return 0;
}
