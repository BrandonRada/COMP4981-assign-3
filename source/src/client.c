//
// Created by brandon-rada on 3/2/25.
//

#include "client.h"
#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int   port      = atoi(argv[2]);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if(connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server. Enter commands:\n");
    communicate_with_server(client_socket);

    close(client_socket);
    return 0;
}

void communicate_with_server(int socket)
{
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while(1)
    {
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);

        if(write(socket, buffer, strlen(buffer)) < 0)
        {
            perror("Write to server failed");
            break;
        }

        if(strncmp(buffer, "exit", 4) == 0)
        {
            printf("Disconnecting...\n");
            break;
        }

        memset(response, 0, BUFFER_SIZE);
        int bytes_received = read(socket, response, BUFFER_SIZE - 1);

        if(bytes_received <= 0)
        {
            printf("Server disconnected.\n");
            break;
        }

        printf("%s\n", response);
    }
}
