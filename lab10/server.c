#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <asm-generic/socket.h>

#define PORT 10086
#define MAX_DATA_SIZE 16384

ssize_t receive_full_data(int new_socket, char* buffer, size_t len) 
{
    ssize_t total_received = 0, bytes_received;

    while (total_received < len) {
        bytes_received = recv(new_socket, buffer + total_received,
                              len - total_received, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }
        total_received += bytes_received;
    }

    return total_received;
}

int main() 
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[MAX_DATA_SIZE] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

#define TEST_COUNT 1000
    for (int i; i < TEST_COUNT; i++) {
        ssize_t ret;
#if 1
        /* use send/recv */
        ret = recv(new_socket, buffer, MAX_DATA_SIZE, 0);
        if (ret != MAX_DATA_SIZE) {
            printf("expect 0x%x, recieved 0x%lx\n",
                   MAX_DATA_SIZE, ret);
            exit(EXIT_FAILURE);
        }
        send(new_socket, buffer, MAX_DATA_SIZE, 0);
#endif

#if 0
        /* use write/read */
        ret = read(new_socket, buffer, MAX_DATA_SIZE);
        if (ret != MAX_DATA_SIZE) {
            printf("expect 0x%x, read 0x%lx\n",
                   MAX_DATA_SIZE, ret);
            exit(EXIT_FAILURE);
        }
        write(new_socket, buffer, MAX_DATA_SIZE);
#endif

#if 0
        /* fix through more interations */
        ret = receive_full_data(new_socket, buffer, MAX_DATA_SIZE);
        if (ret != MAX_DATA_SIZE) {
            printf("expect 0x%x, recieved 0x%lx\n",
                   MAX_DATA_SIZE, ret);
            exit(EXIT_FAILURE);
        }
        send(new_socket, buffer, MAX_DATA_SIZE, 0);
#endif

#if 0
        /* fix through more interations */
        ret = recv(new_socket, buffer, MAX_DATA_SIZE, MSG_WAITALL);
        if (ret != MAX_DATA_SIZE) {
            printf("expect 0x%x, recieved 0x%lx\n",
                   MAX_DATA_SIZE, ret);
            exit(EXIT_FAILURE);
        }
        send(new_socket, buffer, MAX_DATA_SIZE, 0);
#endif
    }
    printf("success for %d iterations\n", TEST_COUNT);

    return 0;
}
