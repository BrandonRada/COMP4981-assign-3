//
// Created by brandon-rada on 3/2/25.
//

#include "server.h"
#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    int                server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t          client_len = sizeof(client_addr);

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    // Bind socket to port
    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if(listen(server_socket, 5) < 0)
    {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    while(1)
    {
        // Accept client connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if(client_socket < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("New client connected!\n");

        // Fork a new process to handle the client
        pid_t pid = fork();
        if(pid == 0)
        {
            // Child process: handle the client
            close(server_socket);
            handle_client(client_socket);
            exit(0);
        }
        else if(pid > 0)
        {
            // Parent process: close the client socket and continue
            close(client_socket);
        }
        else
        {
            perror("Fork failed");
        }
    }

    close(server_socket);
    return 0;
}

// Handles communication with a single client
void handle_client(int client_socket)
{
    char buffer[BUFFER_SIZE];

    while(1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);

        if(bytes_read <= 0)
        {
            printf("Client disconnected.\n");
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0;    // Remove newline character

        // Exit if the client sends "exit"
        if(strcmp(buffer, "exit") == 0)
        {
            printf("Client requested to exit.\n");
            break;
        }

        execute_command(buffer, client_socket);
    }

    close(client_socket);
}

// Executes the command using execv()
void execute_command(char *command, int client_socket)
{
    char *args[BUFFER_SIZE];
    char  full_command[BUFFER_SIZE];
    char *token = strtok(command, " ");
    int   i     = 0;

    // Tokenize command into arguments
    while(token != NULL)
    {
        args[i++] = token;
        token     = strtok(NULL, " ");
    }
    args[i] = NULL;

    // Construct full path for the command (assumes it's in /bin or /usr/bin)
    snprintf(full_command, sizeof(full_command), "/bin/%s", args[0]);

    int pipefd[2];
    pipe(pipefd);

    pid_t pid = fork();
    if(pid == 0)
    {
        // Redirect output to pipe
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // Execute command using execv
        execv(full_command, args);
        perror("Execution failed");
        exit(1);
    }
    else
    {
        close(pipefd[1]);
        char output[BUFFER_SIZE];

        while(read(pipefd[0], output, BUFFER_SIZE) > 0)
        {
            write(client_socket, output, strlen(output));
            memset(output, 0, BUFFER_SIZE);
        }

        close(pipefd[0]);
        wait(NULL);
    }
}
